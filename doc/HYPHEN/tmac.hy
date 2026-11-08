.\" $NHDT-Header: NetHack 5.0 tmac.hy 2026/10/28_20:28:12 NetHack-5.0:1.0 $
.
.\"   HY hyphenation macro used with both .man and .n
.tm IN tmac.hy
.
.\" call as:
.\"  .HY on
.\"  .HY off
.
.	\" groff and AT&T-descended troffs use different hyphenation patterns.
.	\" Don't hyphenate the last word on a page or column, or
.	\" before/after last/first 2 characters of a word.
.de HY
'tm IN HY '\\$1'
'if '\\$1'off' 'hy 12
'if '\\$1'off' 'tm OFF
'if '\\$1'on' 'hy 2
'if '\\$1'on' 'tm ON
..
.
.\" start with hyphenation on
'HY on
.\" .
.\" .de HY
.\" 'tm IN HY '\\$1'
.\" 'ie '\\$1'on' \{
.\" '   tm IN on
.\" '   ie \n(.g 'hy 12
.\" '   el       'hy 14
.\" '\}
.\" 'el 'hy 2
.\" ..
.\" .
.\" .\" start with hyphenation on
.\" 'HY on
