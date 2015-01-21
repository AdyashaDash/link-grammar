# Python bindings for Link Grammar

Description
===========
This directory contains a Python interface to the Link Grammar C library.


Testing
=======
The test collection `tests.py` should run 47 tests, which should all
pass. 


How to use
==========
Parsing simple sentences::

    >>> from linkgrammar import Parser
    >>> p = Parser()
    >>> linkages = p.parse_sent("This is a simple sentence.")
    >>> len(linkages)
    4
    >>> print linkages[0].diagram
    
        +-------------------Xp------------------+
        |              +--------Ost-------+     |
        +------WV------+  +-------Ds------+     |
        +---Wd---+-Ss*b+  |     +----A----+     |
        |        |     |  |     |         |     |
    LEFT-WALL this.p is.v a simple.a sentence.n . 

Additional examples can be found in `examples.py`.
