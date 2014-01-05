SXML
====
A Simple XML token parser written in C.

Inspired by the clean API design used by the [JSON parser JSMN](http://zserge.bitbucket.org/jsmn.html), SXML has the same design goal and features for XML parsing. Go read about JSMN's Philosophy and Features to get an idea of how it differs to other parsers - I'll wait right here.

Features
--------
Here is a list of features SXML shares with JSMN.

* compatible with C89
* no dependencies
* highly portable
* about 360 lines of code
* extremely small code footprint
* API contains only 2 functions
* no dynamic memory allocation
* incremental single-pass parsing

Usage
-----
The header file is heavily commented and should be the first place to look to get started.

Check out the file sxml_test.c for an example of using SXML within a constrained environment with a fixed sized input and output buffer.

Limitations
-----------
In order to remain lightweight the parser has the following limitations:

* Minimal XML validation during parsing
* Limited support for multi-byte encodings beyond those that are [ascii extensions](http://en.wikipedia.org/wiki/Extended_ASCII#Multi-byte_character_encodings) (utf-8 is an ascii extension)
* The greather-than character '>' will need to be escaped in attribute values for correct parsing

Do contact me with suggestions if the limitations above are preventing you from using the parser.

Alternatives
------------
List of alternative lightweight XML parsers considered before writing my own.

* [ezXML](http://ezxml.sourceforge.net/)
* [FastXML](http://codesuppository.blogspot.com/2009/02/fastxml-extremely-lightweight-stream.html)
* [TinyXml](http://www.grinninglizard.com/tinyxml2/)
* [RapidXML](http://rapidxml.sourceforge.net/)
