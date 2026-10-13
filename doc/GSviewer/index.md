# GSviewer user guide

GSviewer displays sequencing alignments, coverage, genomic intervals, and numerical tracks alongside a reference genome and gene annotations.

## Contents

- [Setup through the INI file](#setup-through-the-ini-file)
- [Quick start](#quick-start)
- [Loading files and URLs](#loading-files-and-urls)
- [Tracks and track groups](#tracks-and-track-groups)
- [Genome navigation](#genome-navigation)
- [Bookmarks](#bookmarks)
- [Track settings](#track-settings)
- [Inspecting reads and features](#inspecting-reads-and-features)
- [Reference and gene panels](#reference-and-gene-panels)
- [Sessions](#sessions)
- [Troubleshooting](#troubleshooting)

## Setup through the INI file

Create `GSviewer.ini` next to the GSviewer executable. Use paths to a local reference FASTA and an Ensembl GFF annotation file:

```ini
[General]
reference_genome=C:/genomes/hg38.fa
ensembl_gff=C:/genomes/Homo_sapiens.GRCh38.gff3.gz

# Optional: CA certificate bundle for HTTPS BAM/CRAM access
curl_ca_bundle=C:/certificates/cacert.pem
```

Replace these example paths with your own files. Forward slashes also work in Windows paths.

| Setting | Purpose |
| --- | --- |
| `reference_genome` | Local reference FASTA. Its index must be available as `<reference_genome>.fai`, for example `hg38.fa.fai`. The reference FASTA cannot be loaded through HTTP/HTTPS. |
| `ensembl_gff` | Ensembl GFF file used for gene/transcript display and search. Compressed GFF files are supported. |
| `curl_ca_bundle` | Optional certificate bundle passed to htslib for HTTPS access to BAM/CRAM files. |

Use the same genome assembly for the reference, annotations, and tracks. For CRAM files, use the reference against which the reads were encoded.

GSviewer also reads shared defaults from `settings.ini` next to the executable. User-specific values in `GSviewer_local.ini` in the application's local data directory take precedence over application and shared defaults. GSviewer writes bookmarks and remembered file/session folders to this user-specific file.

Restart GSviewer after changing the reference or GFF configuration. At startup, a **loading gene/transcript Gff file** dialog appears while annotations are loaded. The GFF loader uses Ensembl annotations and includes transcripts marked as GENCODE basic; the gene panel can further restrict the display to GENCODE primary transcripts.

## Quick start

1. Configure the reference FASTA and Ensembl GFF in `GSviewer.ini`, then start GSviewer.
2. Choose **File > Load file** and select an indexed BAM or CRAM file.
3. Enter a gene name such as `BRCA1`, a transcript identifier, or a region such as `chr1:998754-998774` in the search field, then press **Return**.
4. Zoom in until reads and coverage are visible. By default, BAM/CRAM tracks display content for regions of at most 50,000 bases.
5. Click a track and open **Track settings** to adjust its display.
6. Save your work with **Session > Store session**.

## Loading files and URLs

**File > Load file** accepts multiple files. Each file is added in its own track group at the top of the viewer. Use a group's context menu to add files to an existing group instead.

| File type | Display |
| --- | --- |
| `.bam`, `.cram` | A coverage track and a read alignment track. Files must support indexed region queries: provide a matching BAM index, such as `.bam.bai`, or a CRAM index, such as `.cram.crai`. |
| `.bed` | Genomic intervals, with optional region names from the fourth column. |
| `.igv` | Numerical values displayed as a heatmap, bar chart, points, or line plot. This is a numerical track file, not an IGV session file. |

For numerical `.igv` files, provide a tab-separated header and columns for chromosome, start, end, feature name, and numerical value. The fifth column supplies the plotted value. `#track` attributes such as `graphType`, `viewLimits`, and `maxHeightPixels` can supply initial display settings.

Use lowercase file extensions. The loader currently recognizes the extensions listed above; formats such as VCF, bigWig, and compressed BED tracks are not supported by this file loader.

Choose **File > Load URL** to open an HTTP or HTTPS source. For remote BAM/CRAM files, the index must also be accessible, and the server must support the range requests needed for region queries. The URL's path must identify the supported file type; query parameters may follow the filename.

## Tracks and track groups

A **track** displays one kind of data. A **track group** holds several tracks in a separately scrollable area. All groups show the same genomic region, making it possible to compare samples or related data.

- Drag a track by its label on the left to reorder it or move it to another group.
- Drag the horizontal divider between groups to change their heights. The reference/gene panel can also be resized this way.
- Scroll within a group to reach tracks or reads outside its visible area.
- Right-click a track and open the **Track** submenu to **Reload Track**, **Remove Track**, or **Rename Track...**.
- Right-click within a group and open **TrackGroup** to **Add track(s) from file**, **Add track(s) from URL**, **Clear TrackGroup**, or **Remove TrackGroup**.
- Use **Add TrackGroup Above** or **Add TrackGroup Below** in that submenu to create an empty group.

**Clear TrackGroup** removes its tracks; **Remove TrackGroup** removes the group as well. Moving or removing the last track can remove an empty group.

Use **Session > Reload tracks** to refresh all loaded tracks, for example after their source files have changed.

## Genome navigation

### Search

Enter a query in the toolbar's search field and press **Return**:

| Query | Result |
| --- | --- |
| Chromosome, for example `chr1` or `1` | Displays the whole chromosome if it is present in the reference. You can also use the chromosome dropdown. |
| Region, for example `chr1:998754-998774` | Displays the requested interval. Search coordinates are 1-based and inclusive. BED files retain their usual 0-based, half-open input convention. |
| Gene symbol, for example `BRCA1` | Displays the region covered by the gene's loaded transcripts, with 2,000 bases of padding. Gene searches are case-insensitive. |
| Transcript identifier | Displays that loaded transcript with 2,000 bases of padding. Use the transcript identifier present in the GFF, normally an `ENST...` identifier without a version suffix. |

If a gene has several separate transcript regions, GSviewer displays the first region and shows a notice. Search for a particular transcript to choose its location explicitly.

### Mouse and toolbar controls

| Control | Action |
| --- | --- |
| **+** / **-** toolbar buttons | Zoom in or out around the center of the current region. |
| **Ctrl + mouse wheel** over the tracks | Zoom around the position under the pointer. |
| Drag horizontally in the genomic content area | Pan along the genome. Dragging a track label instead moves the track. |
| Click in the chromosome overview or coordinate ruler | Center the view on the clicked coordinate. |
| Drag an interval in the chromosome overview or coordinate ruler | Display the selected interval. |
| Move the pointer over reads, coverage, or the gene panel | Update the coordinate shown in the toolbar. |

The minimum displayed window is 41 bases. Navigation is limited to the reference chromosome boundaries.

## Bookmarks

Open the toolbar's **Region bookmarks** button to:

1. Choose **Store current region** and enter a name to save the displayed interval.
2. Choose a stored bookmark to return to its interval.
3. Choose **Manage**, check unwanted bookmarks, and click the delete button to remove them.

Bookmarks persist in user-specific settings across restarts. They store a name and coordinates, and are separate from session files. They do not store a genome assembly, so use them with the reference assembly for which they were created.

## Track settings

Click a track to select it, then use the toolbar's settings button to show **Track settings**. The panel title identifies the selected track. Hover over a setting to see its description.

With **Auto-apply** enabled, valid changes take effect immediately. Disable it to edit several values before clicking **Apply**. Invalid values are marked in the panel. **Reset** restores the values present when that track was loaded into the settings editor; it is not a factory-default reset.

### Read alignment tracks

| Setting | Effect |
| --- | --- |
| `view as pairs` | Connect paired reads with a line. |
| `show all bases` | Show all read bases instead of only differences from the reference. |
| `show soft-clipped bases` | Include soft-clipped bases in the display. |
| `show secondary alignments` | Include secondary mappings. |
| `group by` | Choose `none` or `phase` to group by haplotype, including unassigned reads. |
| `color by` | Choose `none`, `insert size`, or `read strand`. |
| `visibility range` | Maximum displayed genomic span, in bases. |

### Coverage tracks

| Setting | Effect |
| --- | --- |
| `min display AF for SNP` | Minimum allele frequency required to color the coverage bar for a SNP; default 0.2. |
| `track height` | Height in pixels; default 50. |
| `visibility range` | Maximum displayed genomic span, in bases. |

Alignment and coverage tracks for the same BAM/CRAM source share the visibility range. Changing it in either track updates the other. The default is 50,000 bases; zoom in or increase this setting if content is hidden. Larger ranges can take longer to load and draw.

### BED and numerical tracks

- **BED:** choose `color`, `draw mode` (`collapsed` or `expanded`), and `show region names`. Expanded mode gives overlapping regions separate rows; collapsed mode puts them on a shared row.
- **Numerical IGV:** choose `draw mode` (`heatmap`, `bar chart`, `points`, or `line plot`), axis limits `min` and `max`, and `track_height` in pixels.

Track settings are stored in sessions. Editing them does not change the source data files.

## Inspecting reads and features

Click a read, coverage position, BED interval, or numerical feature to open a detail popup. Popup text can be selected and copied. Read details include coordinates, strand, mate information, insert size, and information about the clicked base or alignment event when available.

Right-click a read and open **Track** for additional actions:

- **Go to mate** navigates to the mate's position when its chromosome is available.
- **Select** highlights the read by name; **Deselect** removes the highlight.

## Reference and gene panels

The panels below the track groups show chromosome context, genomic coordinates, and loaded transcripts. Zoom in to see individual reference bases and translation.

Right-click the gene panel to access:

- **Flip strand:** switch the strand used for sequence/translation display.
- **Show translation:** toggle amino-acid translation.
- **Show GENCODE primary only:** restrict displayed transcripts to those marked as GENCODE primary in the loaded annotations.

## Sessions

Use **Session > Store session** to save an XML file with an `.xml` extension. Use **Session > Load session** to restore it. Loading a session replaces the current track groups.

A session stores the displayed region, track groups and their order, track source paths or URLs, display names, and per-track settings. It references source files rather than embedding their contents, so those files and any indexes must remain accessible. Remote URLs must still be valid when reopening the session.

Reference/GFF configuration, bookmarks, read selections, group heights, and gene-panel display options are not saved in the session. Configure the same reference and annotations before opening a session on another computer.

**Session > Clear session** removes loaded tracks and groups and creates an empty group. It leaves the current genomic region, reference/gene panels, and stored bookmarks available.

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| GSviewer fails at startup | Check `reference_genome`, the adjacent `.fai`, and `ensembl_gff`. GFF loading errors are written to standard error and stop startup. |
| Gene/transcript search does not find a feature | Check the spelling and loaded annotation release. Only transcripts included by the GFF loader can be searched. Try a known genomic region to verify navigation. |
| BAM/CRAM tracks show no content at a wide view | Zoom in below the track's `visibility range`, or increase that setting. |
| BAM/CRAM region loading fails | Check the index, reference chromosome, and file access. For remote files, also check index access and range-request support. |
| HTTPS alignment access fails | Check the URL, credentials or expiry, network access, and certificate configuration. If needed, configure `curl_ca_bundle` and restart GSviewer. |
| CRAM reading fails | Check that the local reference and its `.fai` are available and match the CRAM reference. |
| A reopened session reports errors | Check that its files, indexes, and URLs remain accessible. A session does not copy the data. |
| Data looks displaced or mismatches the reference | Check that every file and annotation uses the same genome assembly. |

Use **Help > About** to find GSviewer and library version information when reporting a problem.
