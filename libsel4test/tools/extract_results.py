#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
import argparse
import bs4
import functools
import re
import sys

SYS_OUT = 'system-out'
XML_SPECIAL_CHARS = {'<': "&lt;", '&': "&amp;", '>': "&gt;", '"': "&quot;", "'": "&apos;"}

TAG_WHITELIST = {
    # Keys are tags to be emitted, values are whether to emit their inner text.
    'error': True,
    'failure': True,
    'testsuite': False,
    'testcase': False,
    SYS_OUT: True,
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
            print(tag, file=f)
        else:
            print('<%s>' % tag.name, file=f)
            text = tag.get_text()
            for ch in text:
                if ch not in XML_SPECIAL_CHARS:
                    f.write(ch)
                else:
                    f.write(XML_SPECIAL_CHARS[ch])
            print('</%s>' % tag.name, file=f)
    else:
        print('<%(name)s %(attrs)s>' % {
            'name': tag.name,
            'attrs': ' '.join(['%s="%s"' % (x[0], x[1]) for x in list(tag.attrs.items())]),
        }, file=f)

        # Recurse for our children.
        list(map(functools.partial(print_tag, f),
                 [x for x in tag.children if isinstance(x, bs4.element.Tag)]))

        print('</%s>' % tag.name, file=f)


def main():
    parser = argparse.ArgumentParser('Cleanup messy XML output from sel4test')
    parser.add_argument('input',
                        nargs='?', help='Input file', type=argparse.FileType('r', errors="ignore"),
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
    regexp = re.compile(r'(<%(top)s>.*</%(top)s>)' % {'top': TOP_TAG}, re.S)
    matches = re.search(regexp, data)
    if not matches or len(matches.groups()) != 1:
        print('Failed to strip leading and trailing garbage', file=sys.stderr)
        return -1
    data = matches.group(0)

    # Dump input data *before* parsing in case we choke during parsing. This
    # means end users have a chance of determining what went wrong from the
    # original output.
    if not args.quiet:
        print(data)

    # Parse the input as HTML even though BS supports XML. It seems the XML
    # parser is a bit more precious about the input.
    try:
        soup = bs4.BeautifulSoup(data, "lxml")
    except Exception as inst:
        print('Failed to parse input: %s' % inst, file=sys.stderr)
        return -1

    try:
        top = soup.find_all(TOP_TAG)[0]
    except Exception as inst:
        print('Failed to find initial %s tag: %s' % (TOP_TAG, inst), file=sys.stderr)
        return -1

    try:
        print_tag(args.output, top)
    except Exception as inst:
        print('While navigating XML: %s' % inst, file=sys.stderr)

    return 0


if __name__ == '__main__':
    sys.exit(main())
