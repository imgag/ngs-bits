FROM mysql:5.7

ENV MYSQL_ALLOW_EMPTY_PASSWORD=true
ENV MYSQL_USER=admin
ENV MYSQL_PASSWORD=1234
ENV MYSQL_DATABASE=ngs
ENV MYSQL_ROOT_PASSWORD=mypass123

COPY ./src/cppNGSD/resources/NGSD_schema.sql /docker-entrypoint-initdb.d/
COPY ./src/cppNGSD/resources/NGSD_initial_data.sql .

CMD ["mysqld"]
