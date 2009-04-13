/*
 *  ecAst.c:resolveName was calculating the nthBlock incorrectly for erased blocks.
 *  ejs --optimize 0 file
 *  the nested lookup of "row" gets the wrong block slot and prints the wrong value
 */
function fun(grid: Array): Void {
    for (let row: Object in grid) {
        for (let name: String in grid[row]) {
            assert(row == 0)
            if (true) {
                assert(row == 0)
            }
        }
    }
}
fun([{count: 23}])
