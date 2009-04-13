/*
 *  Was hoisting "x" when not binding globals. Must not hoist.
 */
    {
        let x = 3
        assert(x == 3)
    }
