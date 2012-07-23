# downloaded from https://bitbucket.org/uchida/sphinx-dollarmathext/src/6377af358bbd/dollarmath.py
#
# dollarmath.py by Akihiro Uchida *public domain*
# the original one is written by Paul Kienzle
# and published as public domain in [sphinx-dev]: $math$ extension
r"""
Allow $math$ markup in text and docstrings, ignoring \$.

The $math$ markup should be separated from the surrounding text by spaces.
To embed markup within a word, place backslash-space before and after.
For convenience, the final $ can be followed by punctuation
(period, comma or semicolon).
"""

import re

dollar_pat = r"(?:^|(?<=\s))[$]([^\n]*?)(?<![\\])[$](?:$|(?=\s|[.,;\\]))"
_dollar = re.compile(dollar_pat)
_notdollar = re.compile(r"\\[$]")

def replace_dollar(content):
    content = _dollar.sub(r":math:`\1`", content)
    content = _notdollar.sub("$", content)
    return content

def rewrite_rst(app, docname, source):
    source[0] = replace_dollar(source[0])

def rewrite_autodoc(app, what, name, obj, options, lines):
    lines[:] = [replace_dollar(L) for L in lines]

def setup(app):
    app.connect('source-read', rewrite_rst)
    if 'autodoc-process-docstring' in app._events:
        app.connect('autodoc-process-docstring', rewrite_autodoc)

def test_dollar():
    samples = {
        "no dollar": "no dollar",
        "$only$": ":math:`only`",
        "$first$ is good": ":math:`first` is good",
        "so is $last$": "so is :math:`last`",
        "and $mid$ too": "and :math:`mid` too",
        "$first$, $mid$, $last$": u":math:`first`, :math:`mid`, :math:`last`",
        "dollar\$ escape": "dollar$ escape",
        "dollar \$escape\$ too": "dollar $escape$ too",
        "emb\ $ed$\ ed": "emb\ :math:`ed`\ ed",
        "$first$a": "$first$a",
        "a$last$": "a$last$",
        "a $mid$dle a": "a $mid$dle a",
    }
    for expr, expect in samples.items():
        assert replace_dollar(expr) == expect

if __name__ == "__main__":
    test_dollar()

