#include "RequestParser.h"
#include <QSet>
#include <cstring>
#include "Settings.h"
#include "Log.h"
#include "Exceptions.h"
#include "HttpUtils.h"
#include "ServerHelper.h"

namespace
{
const QByteArray HEADER_TERMINATOR = QByteArrayLiteral("\r\n\r\n");

// Headers whose values may legitimately contain commas and must NOT be split into a list.
const QSet<QByteArray>& nonListHeaders()
{
	static const QSet<QByteArray> headers {
		"date", "expires", "last-modified", "if-modified-since", "if-unmodified-since",
		"user-agent", "cookie", "set-cookie", "authorization", "referer", "host"
	};
	return headers;
}

inline bool hasBytesAt(const QByteArray& data, int pos, const char* literal, int length)
{
	return pos >= 0 && pos + length <= data.size() && std::memcmp(data.constData() + pos, literal, length) == 0;
}
}

RequestParser::RequestParser()
{
}

HttpRequest RequestParser::parse(QByteArray *request) const
{
	HttpRequest parsed_request;

	// NOTE: if Settings::boolean() hits QSettings/disk, cache this value at startup.
	if (Settings::boolean("show_raw_request", true))
	{
		Log::info(*request);
	}

	// Locate the header block once; only this part is ever split into lines.
	const int header_end = request->indexOf(HEADER_TERMINATOR);
	if (header_end == -1) THROW(Exception, "HTTP header terminator not found");
	const int body_start = header_end + HEADER_TERMINATOR.size();

	QString content_type_header;

	auto addHeaderValue = [&](const QByteArray& name, const QByteArray& value)
	{
		parsed_request.addHeader(name, value);
		if (content_type_header.isEmpty() && name == "content-type") content_type_header = QString::fromUtf8(value);
	};

	// ---- Header block (request line + headers) ----
	int pos = 0;
	bool first_line = true;
	while (pos <= header_end)
	{
		int eol = request->indexOf('\n', pos);
		if (eol == -1 || eol > header_end) eol = header_end;
		const QByteArray line = request->mid(pos, eol - pos).trimmed();
		pos = eol + 1;

		if (first_line)
		{
			first_line = false;

			// "METHOD SP TARGET SP VERSION"
			const int method_end = line.indexOf(' ');
			if (method_end <= 0) THROW(Exception, "Cannot process the request. Probably the URL is missing or incorrect");

			int target_start = method_end + 1;
			while (target_start < line.size() && line[target_start] == ' ') ++target_start;
			int target_end = line.indexOf(' ', target_start);
			if (target_end == -1) target_end = line.size();
			if (target_end <= target_start) THROW(Exception, "Cannot process the request. Probably the URL is missing or incorrect");

			parsed_request.setMethod(inferRequestMethod(line.left(method_end).toUpper()));

			const QByteArray target = line.mid(target_start, target_end - target_start);

			// Path only (query string excluded, so '/' inside parameter values cannot leak into path items).
			const int query_pos = target.indexOf('?');
			const QByteArray raw_path = (query_pos == -1) ? target : target.left(query_pos);

			// Collapse repeated slashes
			QByteArray collapsed;
			collapsed.reserve(raw_path.size());
			for (int i = 0; i < raw_path.size(); ++i)
			{
				if (raw_path[i] == '/' && !collapsed.isEmpty() && collapsed.endsWith('/')) continue;
				collapsed.append(raw_path[i]);
			}

			const QList<QString> path_items = QString::fromUtf8(collapsed).split('/');
			parsed_request.setPrefix(getRequestPrefix(path_items));
			parsed_request.setPath(getRequestPath(path_items));
			parsed_request.setPathItems(getRequestPathParams(path_items));
			parsed_request.setUrlParams(getVariables(getVariableSequence(target)));
			continue;
		}

		if (line.isEmpty()) continue;

		const int header_separator = line.indexOf(':');
		if (header_separator == -1)
		{
			// Preserves old behaviour: lines containing '=' but no ':' are silently ignored.
			if (line.indexOf('=') == -1) THROW(Exception, "Malformed element: " + line);
			continue;
		}

		const QByteArray name = line.left(header_separator).trimmed().toLower();
		const QByteArray value = line.mid(header_separator + 1).trimmed();
		if (value.isEmpty()) continue;

		if (nonListHeaders().contains(name))
		{
			addHeaderValue(name, value);
			continue;
		}

		// List-style header: split on commas
		int start = 0;
		while (true)
		{
			const int comma = value.indexOf(',', start);
			const int end = (comma == -1) ? value.size() : comma;
			const QByteArray item = value.mid(start, end - start).trimmed();
			if (!item.isEmpty()) addHeaderValue(name, item);
			if (comma == -1) break;
			start = comma + 1;
		}
	}

	// ---- Content type ----
	parsed_request.setContentType(ContentType::TEXT_HTML); // default type, if the header is not found

	QByteArray multipart_boundary;
	if (!content_type_header.isEmpty())
	{
		const QList<QString> parts = content_type_header.split(';');
		const QString content_type = parts[0].trimmed();
		const ContentType type = HttpUtils::getContentTypeFromString(content_type);
		parsed_request.setContentType(type);

		if (type == ContentType::MULTIPART_FORM_DATA)
		{
			for (int i = 1; i < parts.count(); ++i)
			{
				const QString param = parts[i].trimmed();
				if (!param.startsWith("boundary=", Qt::CaseInsensitive)) continue;

				QString boundary = param.mid(9);
				boundary.remove('"');
				if (!boundary.isEmpty()) multipart_boundary = "--" + boundary.toUtf8();
				break;
			}
		}
	}

	// ---- Body ----
	if (!multipart_boundary.isEmpty())
	{
		// Parsed in place; the full body is deliberately NOT copied into the request.
		// If other code needs getBody() for multipart requests, add:
		//   parsed_request.setBody(request->mid(body_start));
		parseMultipart(*request, body_start, multipart_boundary, parsed_request);
	}
	else
	{
		const QByteArray body = request->mid(body_start);
		parsed_request.setBody(body);

		if (parsed_request.getContentType() == ContentType::APPLICATION_X_WWW_FORM_URLENCODED)
		{
			parsed_request.setFormUrlEncoded(getVariables(body));
		}
	}

	return parsed_request;
}

void RequestParser::parseMultipart(const QByteArray& data, int body_start, const QByteArray& boundary, HttpRequest& request) const
{
	const QList<int> starts = getBoundaryStartPositions(data, body_start, boundary);
	bool file_taken = false;

	for (int p = 0; p < starts.count(); ++p)
	{
		const int item_start = starts[p] + boundary.size();
		int item_end = (p + 1 < starts.count()) ? starts[p + 1] : data.size();

		// The CRLF right before the next boundary belongs to the framing, not to the part.
		if (item_end >= 2 && hasBytesAt(data, item_end - 2, "\r\n", 2)) item_end -= 2;

		if (item_end < item_start) THROW(Exception, "Invalid multipart boundary positions");

		// Closing boundary ("--" after the marker)
		if (hasBytesAt(data, item_start, "--", 2)) continue;

		// Only the (small) part header block is searched and copied.
		const int part_header_end = data.indexOf(HEADER_TERMINATOR, item_start);
		if (part_header_end == -1 || part_header_end + HEADER_TERMINATOR.size() > item_end) continue;

		const int content_start = part_header_end + HEADER_TERMINATOR.size();
		const int content_length = item_end - content_start;

		const QByteArray part_headers = data.mid(item_start, part_header_end - item_start);
		const bool is_file = part_headers.toLower().contains("content-type");

		if (!is_file)
		{
			// Plain form field
			const QString key = QString::fromUtf8(getPartHeaderParam(part_headers, "name="));
			const QString value = QString::fromUtf8(data.constData() + content_start, content_length).trimmed();
			if (!key.isEmpty() && !value.isEmpty()) request.addFormDataParam(key, value);
		}
		else if (request.getMultipartFileName().isEmpty())
		{
			// Only the first file is kept (as before). The payload is copied exactly once.
			request.setMultipartFileName(QString::fromUtf8(getPartHeaderParam(part_headers, "filename=")));
			request.setMultipartFileContent(data.mid(content_start, content_length));
			file_taken = true;
		}
	}

	Q_UNUSED(file_taken)
}

QList<int> RequestParser::getBoundaryStartPositions(const QByteArray& data, int from, const QByteArray& marker) const
{
	QList<int> positions;
	int pos = from;

	while ((pos = data.indexOf(marker, pos)) != -1)
	{
		const int marker_end = pos + marker.size();
		const bool at_body_start = (pos == from);
		const bool preceded_by_crlf = hasBytesAt(data, pos - 2, "\r\n", 2);
		const bool followed_by_crlf = hasBytesAt(data, marker_end, "\r\n", 2);
		const bool closing_boundary = hasBytesAt(data, marker_end, "--", 2);
		if ((at_body_start || preceded_by_crlf) && (followed_by_crlf || closing_boundary)) positions.append(pos);

		pos = marker_end;
	}

	return positions;
}

// Extracts e.g. name="x" or filename="y" from a part's header block.
// `key` must be lowercase and include the '=' (e.g. "name="). Does not match "filename=" when asked for "name=".
QByteArray RequestParser::getPartHeaderParam(const QByteArray& part_headers, const QByteArray& key) const
{
	const QByteArray lower = part_headers.toLower(); // ASCII lowercase: same length/offsets as the original
	int from = 0;

	while (true)
	{
		const int pos = lower.indexOf(key, from);
		if (pos == -1) return QByteArray();

		const char before = (pos == 0) ? ' ' : lower[pos - 1];
		if (before != ' ' && before != ';' && before != '\t' && before != '\n')
		{
			from = pos + 1;
			continue;
		}

		const int value_start = pos + key.size();
		if (value_start < part_headers.size() && part_headers[value_start] == '"')
		{
			const int end = part_headers.indexOf('"', value_start + 1);
			if (end == -1) return QByteArray();
			return part_headers.mid(value_start + 1, end - value_start - 1);
		}

		int end = value_start;
		while (end < part_headers.size() && part_headers[end] != ';' && part_headers[end] != '\r' && part_headers[end] != '\n') ++end;
		return part_headers.mid(value_start, end - value_start).trimmed();
	}
}

QMap<QString, QString> RequestParser::getVariables(const QByteArray& input) const
{
	QMap<QString, QString> vars;
	const int size = input.size();
	int start = 0;

	while (start < size)
	{
		int amp = input.indexOf('&', start);
		if (amp == -1) amp = size;

		const QByteArray segment = input.mid(start, amp - start);
		const int eq = segment.indexOf('='); // split on the FIRST '=' only, so values may contain '='
		if (eq > 0)
		{
			vars.insert(QString::fromUtf8(segment.constData(), eq), QString::fromUtf8(segment.constData() + eq + 1, segment.size() - eq - 1));
		}

		start = amp + 1;
	}

	return vars;
}

QByteArray RequestParser::getVariableSequence(const QByteArray& url) const
{
	const int q = url.indexOf('?');
	if (q == -1) return QByteArray();
	return url.mid(q + 1);
}

QString RequestParser::getRequestPrefix(const QList<QString>& path_items) const
{
	if (path_items.count() > 1)
	{
		return ServerHelper::getUrlWithoutParams(path_items[1]);
	}
	return "";
}

QString RequestParser::getRequestPath(const QList<QString>& path_items) const
{
	if (path_items.count() > 2)
	{
		return ServerHelper::getUrlWithoutParams(path_items[2]);
	}
	return "";
}

QList<QString> RequestParser::getRequestPathParams(const QList<QString>& path_items) const
{
	QList<QString> params {};
	for (int p = 3; p < path_items.count(); ++p)
	{
		QString current_item = path_items[p].trimmed();
		const int param_separator = current_item.indexOf('?');
		if (param_separator > -1) current_item = current_item.left(param_separator);
		if (current_item.isEmpty()) continue;
		params.append(current_item);
	}
	return params;
}

RequestMethod RequestParser::inferRequestMethod(const QByteArray& input) const
{
	const QByteArray method = input.toUpper();

	if (method == "GET") return RequestMethod::GET;
	if (method == "POST") return RequestMethod::POST;
	if (method == "DELETE") return RequestMethod::DELETE;
	if (method == "PUT") return RequestMethod::PUT;
	if (method == "PATCH") return RequestMethod::PATCH;
	if (method == "HEAD") return RequestMethod::HEAD;

	THROW(ArgumentException, "Unsupported request method");
}
