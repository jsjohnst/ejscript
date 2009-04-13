/*
 *  Test regular expressions
 */

if (Config.RegularExpressions) {
    re = new RegExp("def")
    assert(re is RegExp)

    /*
     *  "test" method
     */
    assert(re.test("abcdef"))
    assert(!re.test("xyz"))

    /*
     *  Basic exec
     */
    assert(re.exec("abcdef") == "def")
    assert(re.exec("xyz") == null)

    /*
     *  With global flag
     */
    re = /def/g
    assert(re.exec("abcdef") == "def")
    assert(re.exec("xyz") == null)

    re = /(a)(b)(c)/g
    assert((re.exec("abc def abc")).length == 4)
    assert(re.exec("abc def abc") == "abc,a,b,c")

    /*
     *  Case insensitive matching
     */
    assert(/def/.test("def"))
    assert(!/def/.test("DEF"))
    assert(/def/i.test("DEF"))

    /*
     *  toString
     */
    assert(/def/ == "/def/")
    assert(/def/g == "/def/g")


    re = /def/g
    assert(re.global)
    assert(!re.ignoreCase)
    assert(!re.multiline)
    assert(!re.sticky)
    assert(/def/y.sticky)
    assert(/def/m.multiline)
    assert(/def/i.ignoreCase)

    re = /def/
    re.exec("abcdefghi")
    assert(re.lastIndex == 6)
    assert(re.start == 3)
    re.lastIndex = 0
    assert(re.lastIndex == 0)
    assert(re.matched == "def")

    /*
        re.matched

        Delegated
            re.split
            re.replace
        re.replace(re, "$2 $1")
     */
}
