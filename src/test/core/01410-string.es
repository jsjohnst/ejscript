/*
 *  Test String regular expressions methods
 */

if (Config.RegularExpressions) {
    /*
     *  contains
     */
    assert("abc def ghi".contains(/def/))
    assert(!"abc def ghi".contains(/xyz/))

    /*
     *  Match
     */
    assert("abcdefghi".match(/abc/) == "abc")
    assert("abc def ghi ABC DEF GHI".match(/([a-z])+/g) == "abc,def,ghi")


    /*
     *  Replace
     */
    assert("abc def ABC DEF".replace(/([a-z ]+) ([A-Z ]+)/g, '$2 $1') == "ABC DEF abc def")
    assert("abc def ghi ABC DEF GHI".replace(/([a-c]+) ([d-f]+)/g, '_$2 $1_') == "_def abc_ ghi ABC DEF GHI")
    assert("abc".replace(/b/,"$$") == "a$c")

    /*
     *  Search
     */
    assert("hello world".search(/[w]/) == 6)
    assert("hello world".search(/[zzz]/) == -1)

    /*
     *  Split
     */
    results = "abc def ghi ABC DEF GHI".match(/([a-zA-Z]+)/g)
    assert(results.length == 6)
    assert(results[0] == "abc")
}
