#!/usr/bin/env python
#
#  Copyright 2017, Data61
#  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
#  ABN 41 687 119 230.
#
#  This software may be distributed and modified according to the terms of
#  the BSD 2-Clause license. Note that NO WARRANTY is provided.
#  See "LICENSE_BSD2.txt" for details.
#
#  @TAG(DATA61_BSD)
import argparse, bs4, functools, re, sys

SYS_OUT = 'system-out'
XML_SPECIAL_CHARS = {'<':"&lt;",'&':"&amp;",'>':"&gt;",'"':"&quot;","'":"&apos;"}

TAG_WHITELIST = {
    # Keys are tags to be emitted, values are whether to emit their inner text.
    'error':True,
    'failure':True,
    'testsuite':False,
    'testcase':False,
    SYS_OUT:True,
}

TOP_TAG = 'testsuite'

def print_tag(f, tag):
    assert isinstance(tag, bs4.element.Tag)

    # Skip non-whitelisted tags.
    if tag.name not in TAG_WHITELIST:
        return

    # If we want the inner text, just blindly dump the soup.
    if TAG_WHITELIST[tag.name]:
        if tag.name != SYS_OUT:
            print >>f, tag
        else:
            print >>f, '<%s>' % tag.name
            text = tag.get_text()
            for ch in text:
                if ch not in XML_SPECIAL_CHARS:
                    f.write(ch)
                else:
                    f.write(XML_SPECIAL_CHARS[ch])
            print >>f, '</%s>' % tag.name
    else:
        print >>f, '<%(name)s %(attrs)s>' % {
            'name':tag.name,
            'attrs':' '.join(map(lambda x: '%s="%s"' % (x[0], x[1]),
                tag.attrs.items())),
        }

        # Recurse for our children.
        map(functools.partial(print_tag, f),
            filter(lambda x: isinstance(x, bs4.element.Tag),
                tag.children))

        print >>f, '</%s>' % tag.name

def main():
    parser = argparse.ArgumentParser('Cleanup messy XML output from sel4test')
    parser.add_argument('input',
        nargs='?', help='Input file', type=argparse.FileType('r'),
        default=sys.stdin)
    parser.add_argument('output',
        nargs='?', help='Output file', type=argparse.FileType('w'),
        default=sys.stdout)
    parser.add_argument('--quiet', '-q',
        help='Suppress unmodified output to stdout', action='store_true',
        default=False)
    args = parser.parse_args()

    data = args.input.read()

    # Strip trailing crap around the XML we want to parse. Without this, even
    # BeautifulSoup sometimes backs away in horror.
    regexp = re.compile(r'(<%(top)s>.*</%(top)s>)' % {'top':TOP_TAG}, re.S)
    matches = re.search(regexp, data)
    if not matches or len(matches.groups()) != 1:
        print >>sys.stderr, 'Failed to strip leading and trailing garbage'
        return -1
    data = matches.group(0)

    # Dump input data *before* parsing in case we choke during parsing. This
    # means end users have a chance of determining what went wrong from the
    # original output.
    if args.quiet:
        print data

    # Parse the input as HTML even though BS supports XML. It seems the XML
    # parser is a bit more precious about the input.
    try:
        soup = bs4.BeautifulSoup(data, "lxml")
    except Exception as inst:
        print >>sys.stderr, 'Failed to parse input: %s' % inst
        return -1

    try:
        top = soup.find_all(TOP_TAG)[0]
    except Exception as inst:
        print >>sys.stderr, 'Failed to find initial %s tag: %s' % (TOP_TAG, inst)
        return -1

    try:
        print_tag(args.output, top)
    except Exception as inst:
        print >>sys.stderr, 'While navigating XML: %s' % inst

    return 0

if __name__ == '__main__':
    sys.exit(main())
