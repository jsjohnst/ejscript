/**
 *	HtmlConnector.es -- Basic HTML control connector
 */

module ejs.web {

    use module ejs.db

	/**
	 *	The Html Connector provides bare HTML encoding of Ejscript controls
     *	TODO - should actually implement the ViewConnector
	 */
	class HtmlConnector {

        use default namespace "ejs.web"

        /*
         *  Options to implement:
         *      method
         *      update
         *      confirm     JS confirm code
         *      condition   JS expression. True to continue
         *      success
         *      failure
         *      query
         *
         *  Not implemented
         *      submit      FakeFormDiv
         *      complete
         *      before
         *      after
         *      loading
         *      loaded
         *      interactive
         */
        /**
         *  @duplicate View.aform
         */
		function aform(record: Object, url: String, options: Object): Void {
            if (options.id == undefined) {
                options.id = "form"
            }
            onsubmit = ""
            if (options.condition) {
                onsubmit += options.condition + ' && '
            }
            if (options.confirm) {
                onsubmit += 'confirm("' + options.confirm + '"); && '
            }
            onsubmit = '$.ajax({ ' +
                'url: "' + url + '", ' + 
                'type: "' + options.method + '", '

            if (options.query) {
                onsubmit += 'data: ' + options.query + ', '
            } else {
                onsubmit += 'data: $("#' + options.id + '").serialize(), '
            }

            if (options.update) {
                if (options.success) {
                    onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); ' + 
                        options.success + '; }, '
                } else {
                    onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); }, '
                }
            } else if (options.success) {
                onsubmit += 'success: function(data) { ' + options.success + '; } '
            }
            if (options.error) {
                onsubmit += 'error: function(data) { ' + options.error + '; }, '
            }
            onsubmit += '}); return false;'

            write('<form action="' + "/User/list" + '"' + getOptions(options) + "onsubmit='" + onsubmit + "' >")
        }


        /*
         *  Extra options:
         *      method
         *      update
         *      confirm     JS confirm code
         *      condition   JS expression. True to continue
         *      success
         *      failure
         *      query
         *
         */
        /**
         *  @duplicate View.alink
         */
		function alink(text: String, url: String, options: Object): Void {
            if (options.id == undefined) {
                options.id = "alink"
            }
            onclick = ""
            if (options.condition) {
                onclick += options.condition + ' && '
            }
            if (options.confirm) {
                onclick += 'confirm("' + options.confirm + '"); && '
            }
            onclick = '$.ajax({ ' +
                'url: "' + url + '", ' + 
                'type: "' + options.method + '", '

            if (options.query) {
                'data: ' + options.query + ', '
            }

            if (options.update) {
                if (options.success) {
                    onclick += 'success: function(data) { $("#' + options.update + '").html(data); ' + 
                        options.success + '; }, '
                } else {
                    onclick += 'success: function(data) { $("#' + options.update + '").html(data); }, '
                }
            } else if (options.success) {
                onclick += 'success: function(data) { ' + options.success + '; } '
            }
            if (options.error) {
                onclick += 'error: function(data) { ' + options.error + '; }, '
            }
            onclick += '}); return false;'

            write('<a href="' + options.url + '"' + getOptions(options) + "onclick='" + onclick + "' >" + text + '</a>')
		}


        /**
         *  @duplicate View.button
         */
		function button(value: String, buttonName: String, options: Object): Void {
            write('<input name="' + buttonName + '" type="submit" value="' + value + '"' + getOptions(options) + ' />')
        }


        /**
         *  @duplicate View.buttonLink
         */
		function buttonLink(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"><button>' + text + '</button></a>')
        }


        /**
         *  @duplicate View.chart
         */
		function chart(data: Array, options: Object): Void {
            //  TODO
            throw 'HtmlConnector control "chart" not implemented.'
		}


        /**
         *  @duplicate View.checkbox
         */
		function checkbox(name: String, value: String, submitValue: String, options: Object): Void {
            let checked = (value == submitValue) ? ' checked="yes" ' : ''
            write('<input name="' + name + '" type="checkbox" "' + getOptions(options) + checked + 
                '" value="' + submitValue + '" />')
        }


        /**
         *  @duplicate View.endForm
         */
		function endform(): Void {
            write('</form>')
        }


        /**
         *  TODO  - how to make this style-able?
         */
		function flash(kind: String, msg: String, options: Object): Void {
            write('<div' + getOptions(options) + '>' + msg + '</div>')
            if (kind == "inform") {
                //  TODO - should be based on ID
                write('<script>$(document).ready(function() {
                        $("div.flashInform").animate({opacity: 1.0}, 2000).hide("slow");});
                    </script>')
            }
		}


        /**
         *  @duplicate View.form
         */
		function form(record: Object, url: String, options: Object): Void {
            write('<form action="' + url + '"' + getOptions(options) + '>')
//          write('<input name="id" type="hidden" value="' + record.id + '" />')
        }


        /**
         *  @duplicate View.image
         */
        function image(src: String, options: Object): Void {
			write('<img src="' + src + '"' + getOptions(options) + '/>')
        }


        /**
         *  @duplicate View.label
         */
        function label(text: String, options: Object): Void {
            write('<span ' + getOptions(options) + ' type="' + getTextKind(options) + '">' +  text + '</span>')
        }


        /**
         *  @duplicate View.link
         */
		function link(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"' + getOptions(options) + '>' + text + '</a>')
		}


        /**
         *  @duplicate View.extlink
         */
		function extlink(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"' + getOptions(options) + '>' + text + '</a>')
		}


        /**
         *  @duplicate View.list
         */
		function list(name: String, choices: Object, defaultValue: String, options: Object): Void {
            write('<select name="' + name + '" ' + getOptions(options) + '>')
            let isSelected: Boolean
            let i = 0
            for each (choice in choices) {
                if (choice is Array) {
                    isSelected = (choice[0] == defaultValue) ? 'selected="yes"' : ''
                    write('  <option value="' + choice[0] + '"' + isSelected + '>' + choice[1] + '</option>')
                } else {
                    if (choice && choice.id) {
                        for (field in choice) {
                            isSelected = (choice.id == defaultValue) ? 'selected="yes"' : ''
                            if (field != "id") {
                                write('  <option value="' + choice.id + '"' + isSelected + '>' + choice[field] + '</option>')
                            }
                        }
                    } else {
                        isSelected = (i == defaultValue) ? 'selected="yes"' : ''
                        write('  <option value="' + i + '"' + isSelected + '>' + choice + '</option>')
                    }
                }
                i++
            }
            write('</select>')
        }


        /**
         *  @duplicate View.mail
         */
		function mail(name: String, address: String, options: Object): Void  {
			write('<a href="mailto:' + address + '" ' + getOptions(options) + '>' + name + '</a>')
		}


        /**
         *  @duplicate View.progress
         */
		function progress(data: Array, options: Object): Void {
            write('<p>' + data + '%</p>')
		}


        //  Emit: <input name ="model.name" id="id" class="class" type="radio" value="text"
        /**
         *  @duplicate View.radio
         */
        function radio(name: String, selected: String, choices: Object, options: Object): Void {
            let checked: String
            if (choices is Array) {
                for each (v in choices) {
                    checked = (v == selected) ? "checked" : ""
                    write(v + ' <input type="radio" name="' + name + '"' + getOptions(options) + 
                        ' value="' + v + '" ' + checked + ' />\r\n')
                }
            } else {
                for (item in choices) {
                    checked = (choices[item] == selected) ? "checked" : ""
                    write(item + ' <input type="radio" name="' + name + '"' + getOptions(options) + 
                        ' value="' + choices[item] + '" ' + checked + ' />\r\n')
                }
            }
        }


		/** 
		 *	@duplicate View.script
		 */
		function script(url: String, options: Object): Void {
            write('<script src="' + url + '" type="text/javascript"></script>\r\n')
		}


        /**
         *  @duplicate View.status
         */
		function status(data: Array, options: Object): Void {
            write('<p>' + data + '</p>\r\n')
        }


		/** 
		 *	@duplicate View.stylesheet
		 */
		function stylesheet(url: String, options: Object): Void {
            write('<link rel="stylesheet" type="text/css" href="' + url + '" />\r\n')
		}


        /**
         *  @duplicate View.tabs
         */
		function tabs(tabs: Array, options: Object): Void {
            write('<div class="menu">')
            write('<ul>')
            for each (t in tabs) {
                for (name in t) {
                    let url = t[name]
                    write('<li><a href="' + url + '">' + name + '</a></li>\r\n')
                }
            }
            write('</ul>')
            write('</div>')
        }


        /**
         *  @duplicate View.table
         */
		function table(data: Array, options: Object = {}): Void {
            //  TODO - should take a model
			if (data == null || data.length == 0) {
				write("<p>No Data</p>")
				return
			}

            if (options.title) {
                write('    <h2 class="ejs tableHead">' + options.title + '</h2>')
            }

/*
            if (options.filter) {
                //  TODO - should we be using options and getOptions here?
                url = view.makeUrl(controller.actionName, "", options)
                write('<form action="' + url + '"' + getOptions(options) + '>')
                write('<input type="text" name="ejs::table" id="" class="ejs tableHead">')
                write('</form>')
            }
*/
			write('<table ' + getOptions(options) + '>')

			write('  <thead class="ejs">')
			write('  <tr>')

            let line: Object = data[0]
            let columns: Object = options["columns"]

            if (columns) {
                for (name in columns) {
                    let column = columns[name]
                    if (line[name] == undefined && column.render == undefined) {
                        throw new Error("Can't find column \"" + name + "\" in data set: " + serialize(line))
                        columns[name] = null
                    }
                }

                /*
                 *  Emit column headings
                 */
                for (let name in columns) {
                    if (name == null) {
                        continue
                    }
                    let header: String

                    //  TODO compiler BUG header = (columns[name].header) ? (columns[name].header) : name.toPascal()
                    if (columns[name].header == undefined) {
                        header = name.toPascal()
                    } else {
                        header = columns[name].header
                    }
                    let width = (columns[name].width) ? ' width="' + columns[name].width + '"' : ''
                    write('    <th class="ejs"' + width + '>' + header + '</th>')
                }

            } else {
                columns = {}
                for (let name in line) {
                    if (name == "id" && !options.showId) {
                        continue
                    }
                    write('    <th class="ejs">' + name.toPascal() + '</th>')
                    columns[name] = {}
                }
            }

			write("  </tr>\r\n</thead>")

            let row: Number = 0
			for each (let r: Object in data) {
				write('  <tr class="ejs">')
                let url: String = null
                if (options.click) {
                    url = view.makeUrl(options.click, r.id, options)
                }
                //  TODO - should implement styleOddRow, styleEvenRow on rows not on cells
                //  TODO - should implement styleCell
                let style = (row % 2) ? "oddRow" : "evenRow"
				for (name in columns) {
                    if (name == null) {
                        continue
                    }

                    let column = columns[name]
                    let cellStyle: String

                    if (column.style) {
                        cellStyle = style + " " + column.style
                    } else {
                        cellStyle = style
                    }
                    //  TODO OPT
                    data = view.getValue(r, name, { render: column.render } )
                    if (url) {
//  TODO - what other styles should this be?
//  TODO - better to use onclick
                        write('    <td class="ejs ' + cellStyle + '"><a href="' + url + '">' + data + '</a></td>')
                    } else {
                        write('    <td class="ejs ' + cellStyle + '">' + data + '</td>')
                    }
				}
                row++
				write('  </tr>')
			}
			write('</table>')
		}


        //  Emit: <input name ="model.name" id="id" class="class" type="text|hidden|password" value="text"
        /**
         *  @duplicate View.text
         */
        function text(name: String, value: String, options: Object): Void {
            write('<input name="' + name + '" ' + getOptions(options) + ' type="' + getTextKind(options) + 
                '" value="' + value + '" />')
        }


        // Emit: <textarea name ="model.name" id="id" class="class" type="text|hidden|password" value="text"
        /**
         *  @duplicate View.textarea
         */
        function textarea(name: String, value: String, options: Object): Void {
            numCols = options.numCols
            if (numCols == undefined) {
                numCols = 60
            }
            numRows = options.numRows
            if (numRows == undefined) {
                numRows = 10
            }
            write('<textarea name="' + name + '" type="' + getTextKind(options) + '" ' + getOptions(options) + 
                ' cols="' + numCols + '" rows="' + numRows + '">' + value + '</textarea>')
        }


        /**
         *  @duplicate View.tree
         */
        function tree(data: Array, options: Object): Void {
            throw 'HtmlConnector control "tree" not implemented.'
        }


        private function getTextKind(options): String {
            var kind: String

            if (options.password) {
                kind = "password"
            } else if (options.hidden) {
                kind = "hidden"
            } else {
                kind = "text"
            }
            return kind
        }


		private function getOptions(options: Object): String {
            return view.getOptions(options)
        }


        private function write(str: String): Void {
            view.write(str)
        }
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
