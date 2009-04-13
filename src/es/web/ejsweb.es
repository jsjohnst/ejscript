#!/Users/mob/hg/ejs/bin/ejs

/*
 *  ejsweb.es -- Ejscript web framework generator. This generates, compiles, cleans and packages Ejscript web applications. 
 *  TODO - create constant for '.es' '.ejs' 
 */

GC.enabled = false

use module ejs.db

class EjsWeb {

    private const DIR_PERMS: Number = 0775
    private const FILE_PERMS: Number = 0666
    private const RC: String = ".ejsrc"
    private const DefaultLayout: String = "views/layouts/default.ejs"
    private const NextMigration: String = ".ejs/nextMigration"

    private var applyMigration: Boolean = false
    private var appName: String
    private var command: String 
    private var config: Object
    private var db: Database
    private var dbPath: String
    private var debug: Boolean = false
    private var keep: Boolean = false
    private var layoutPage: String = DefaultLayout
    private var mode: String = "debug"
    private var overwrite: Boolean = false
    private var reverse: Boolean = false
    private var verbose: Boolean = true
    private var compiler: String
    private var ejsweb: String

    function EjsWeb() {
        config = loadEcf("config/config.ecf", false)
        if (config) {
            mode = config.app.mode
        } else {
            config = {}
        }
        ejsweb = App.args[0]
        if (ejsweb == "ejsweb") {
            compiler = "ejsc"
        } else {
            compiler = "ajsc"
        }
        if (config.compiler == undefined) {
            config.compiler = {}
            config.compiler[mode] = {}
            config.compiler[mode].command = '"' + App.dir + '/' + compiler + '" --debug --web'
        }
    }


    /*
     *  Load the various default files
     */
    function loadDefaults(): Void {

        loadConfigFile(RC, "defaults") || loadConfigFile("~/" + RC, "defaults")
    }


    /*
     *  Parse args and invoke required commands
     */
    function parseArgs(args: Array): Boolean {
        let cmdArgs: Array

        for (let i: Number = 1; i < args.length; i++) {
            switch (args[i]) {
            case "-a":
            case "--apply":
                applyMigration = true
                break

            case "--debug":
                debug = true
                break

            case "-k":
            case "--keep":
                keep = true
                break

            case "--layout":
                layoutPage = args[++i]
                break

            case "--overwrite":
                overwrite = true
                break

            case "--reverse":
                reverse = true
                break

            case "-q":
            case "--quiet":
                verbose = false
                break

            case "-v":
            case "--verbose":
                verbose = true
                break

            default:
                if (args[i].startsWith("-")) {
                    usage()
                    return false
                }
                cmdArgs = args.slice(i)
                //  TODO BUG. Break not working
                i = 9999
                break
            }
        }

        if (cmdArgs == null || cmdArgs.length == 0) {
            usage()
            return false
        }

        let rest: Array = cmdArgs.slice(1)
        let cmd: String = cmdArgs[0].toLower()

        checkApp(cmd, rest)

        switch (cmd) {
        case "browse":
            browse(rest)
            break

        case "clean":
            clean(rest)
            break

        case "compile":
            compile(rest)
            break

        case "console":
            console(rest)
            break

        case "deploy":
            deploy(rest)
            break

        case "install":
            install(rest)
            break

        case "generate":
            generate(rest)
            break

        case "migrate":
            migrate(rest)
            break

        case "run":
            run(rest)
            break

        default:
            return false
        }

        if (applyMigration) {
            migrate()
        }

        return true
    }


    function usage(): Void {
        print("\nUsage: " + App.args[0] + " [options] [commands] ...\n" +
            "  Options:\n" + 
            "    --apply                      # Apply migrations\n" + 
            "    --database [sqlite | mysql]  # Sqlite only currently supported adapter\n" + 
            "    --keep\n" + 
            "    --layout layoutPage\n" + 
            "    --reverse                    # Reverse generated migrations\n" + 
            "    --overwrite\n" + 
            "    --quiet\n" + 
            "    --verbose (default)\n")

        let pre = "    " + App.args[0] + " "
        print("  Commands:\n" +
            pre + "clean\n" +
            pre + "compile [all | app | controller names | model names | view names]\n" +
            pre + "compile path/name.ejs ...\n" +
            pre + "generate app name\n" + 
            pre + "generate controller name [action [, action] ...]\n" + 
            pre + "generate migration description model [field:type [, field:type]...]\n" +
            pre + "generate model name [field:type [, field:type]...]\n" +
            pre + "generate scaffold model [field:type [, field:type]...\n" +
            pre + "migrate [forward|backward|NNN]\n" +
            pre + "run" +
            "")

        /*
         *  TODO
            pre + "dbconsole \n" +             # sqlite
            pre + "console \n" +               # with ejs.db, ejs.web and app loaded (all models, all controllers)
            pre + "generate package\n" +
            pre + "install (not implemented yet)\n" +
            pre + "mode [development | test | production] (not implemented yet)\n" +
            pre + "deploy path.zip (not implemented yet)\n" +
        */
        App.exit(1)
    }

    /*
     *  TODO list
     *  rake db:create - Create database (create dev databases)
     *  rake db:create:all - Create database (create dev, test and production databases)
     *  script/generate model task name:string priority:integer
     *      def self.up
     *      create_table :tasks do |t|
     *          t.string :name
     *          t.integer :priority, :postion
     *          t.timestamps
     *      end
     *  rake db:migrate
     *
     *  Add/remove columns
     *
     *  script/generate migration add_description_to_task description:text 
     *  script/generate migration remove_description_from_task description:text 
     *
     *  script/dbconsole
     *      .tables
     *      select * from schem_migrations
     *  20080515005842
     *  20080515005842
     *
     *
     *  change_table :products do |t|
     *      t.rename
     *      t.change
     *      t.remove
     *      t.integer 
     */


    function clean(args: Array): Void {
        let files: Array = glob(".", /\.mod$/)
        trace("[CLEAN]", files)
        for each (f in files) {
            rm(f)
        }
    }

    //  TODO - refactor and reorder this. Perhaps move compile, generate, migrate etc into separate files

    function compile(args: Array): Void {
        var files: Array

        if (args.length == 0) {
            args.append("everything")
        }
        let kind: String = args[0].toLower()
        let rest: Array = args.slice(1)

//      loadConfigFile("config/compiler.ecf", "compiler", false)

        let ejspat = /\.ejs$/
        let pat = /\.es$/

        switch (kind) {
        case "everything":
            /*
             *  Build all items but NOT as one module
             */
            buildApp()
            for each (name in glob("controllers", pat)) {
                buildController(name)
            }
            files = glob("views", ejspat)
            for each (name in files) {
                buildView(name)
            }
            files = glob("web", ejspat)
            layoutPage = undefined;
            for each (name in files) {
                buildWebPage(name)
            }
            break

        case "all":
            /*
             *  Build entire app as one module
             */
            files = glob("config", pat) + glob("src", pat) + glob("controllers", pat) + glob("views", pat) + 
                glob("models", pat)
            webFiles = glob("web", ejspat)
            layoutPage = undefined
            for each (name in webFiles) {
                files.append(buildWebPage(name, false))
            }
            buildFiles("App.mod", files)
            break

        case "app":
            /*
             *  Build app portions. This includes all config, src, models and BaseController
             */
            buildApp()
            break

        case "controller":
        case "controllers":
            /*
             *  Build controllers
             */
            if (rest.length == 0) {
                for each (name in glob("controllers", pat)) {
                    buildController(name)
                }
            } else {
                for each (name in rest) {
                    buildController(name)
                }
            }
            break

        case "model":
        case "models":
            throw "WARNING: models must be built with the app. Use \"" + ejsweb + " compile app\""
            /*
             *  Build models
             */
            if (rest.length == 0) {
                for each (name in glob("models", pat)) {
                    buildModel(name)
                }
            } else {
                for each (name in rest) {
                    buildModel(name)
                }
            }
            break

        case "view":
        case "views":
            /*
             *  Comile views
             */
            if (rest.length == 0) {
                for each (view in glob("views", ejspat)) {
                    buildView(view)
                }
            } else {
                for each (view in rest) {
                    buildView(view)
                }
            }
            break

        default:
            for (f in args) {
                let file: String = args[0]
                if (extension(file) == '.ejs') {
                    layoutPage = undefined
                    buildWebPage(file)
                } else if (extension(file) == '.es') {
                    build(file)
                } else {
                    throw new ArgError("Unknown compile category: " + kind)
                }
            }
        }
    }


    /*
        use module ejs.db
        load("models/User.es");
        db = Database.defaultDatabase = new Database("db/todo.sdb")
        grid = User.find(3)
        dump(grid)
    */
    function console(args: Array): Void {
        // cmd = 'ejs --use "' + appName + '"'
        let cmd = "ejs"
        //  TODO - this won't work without stdin
        System.run(cmd)
    }


    function buildController(file: String) {
        let testFile = "controllers/" + file.toPascal() + ".es"
        if (exists(testFile)) {
            file = testFile
        }
        if (!file.startsWith("controllers/")) {
            throw "File \"" + file + "\" is not a controller"
        }
        if (!exists(file)) {
            throw "Controller : \"" + file + "\" does not exist"   
        }
        if (file == "controllers/Base.es") {
            return
        }
        build(file)
    }


    function buildModel(file: String) {
        let testFile = "models/" + file.toPascal() + ".es"
        if (exists(testFile)) {
            file = testFile
        }
        if (!file.startsWith("models/")) {
            throw "File \"" + file + "\" is not a controller"
        }
        if (!exists(file)) {
            throw "Model : \"" + file + "\" does not exist"   
        }
        build(file)
    }


    function buildView(file: String) {
        if (file.contains(/^views\/layouts\//)) {
            /*
             *  Skip layouts
             */
            return
        }
        if (!file.startsWith("views/")) {
            throw "File \"" + file + " \" is not a view"
        }
        buildWebPage(file, true, true)
    }


    function buildWebPage(file: String, compile: Boolean = true, isView: Boolean = false): String {

        let ext = extension(file)
        if (ext == "") {
            file += ".ejs"
        } else if (ext != ".ejs") {
            throw "File is not an Ejscript web file: " + file
        }
        if (!exists(file)) {
            if (ext) {
                throw "Can't find ejs page: " + file
            } else {
                throw "Can't find view file: " + file
            }
        }

        let sansExt = file.replace(/.ejs$/,"")
        let controller: String
        let controllerMod: String
        let controllerSource: String
        let controllerPrefix: String
        let viewName: String

        //  TODO - refactor this logic
        if (isView) {
            controller = getNthSegment(sansExt, 1).toPascal()
            viewName = basename(sansExt)

        } else {
            // viewName = sansExt.split(/(\\|\/)+/g).slice(1).join("_")
            viewName = sansExt.replace(/(\\|\/)+/g, "_")
            if (exists("config/compiler.ecf")) {
                controllerPrefix = "Base" + "_"
            } else {
                controllerPrefix = "_Solo" + "_"
            }
        }

        /*
         *  Ensure the corresponding controller (if there is one) is built first
         */
        controllerMod = "controllers/" + controller + ".mod"
        controllerSource = "controllers/" + controller + ".es"

        if (isView && exists(controllerSource)) {
            if (!exists(controllerMod)) {
                build("controllers/" + controller + ".es")
            }
            controllerPrefix = controller + "_"
        }

        /*
         *  Parse the ejs file and create an ".es" equivalent
         */
        trace("[PARSE]", file)
        let ep: EjsParser = new EjsParser
        results = Templates.ViewHeader + ep.parse(file, App.workingDir, layoutPage) + Templates.ViewFooter
        results = results.replace(/\${CONTROLLER}/g, controllerPrefix)
        results = results.replace(/\${VIEW}/g, viewName)

        let esfile = sansExt + ".es"
        File.put(esfile, 0664, results)

        if (compile) {
            let out = sansExt + ".mod"
            rm(out)

            let cmd: String = getCompilerPath()
            if (exists(controllerMod)) {
                cmd += " --out " + out + " App.mod " + controllerMod + " " + esfile
            } else if (appName) {
                cmd += " --out " + out + " App.mod " + esfile
            } else {
                cmd += " --out " + out + " " + esfile
            }

            trace("[BUILD]", cmd)
            command(cmd)

            if (!exists(out)) {
                throw "Compilation failed for " + out + "\n" + results
            }
            if (!keep) {
                rm(esfile)
            }
        }
        return esfile
    }


	function getCompilerPath(): String {
		let cmd = config.compiler[mode].command
		if (cmd.trim('"').match(/^\/|^[a-zA-Z]:\//)) {
			return cmd
		}
		parts = cmd.split(" ")
		cmd = '"' + App.dir + '/' + parts[0] + '" ' + parts.slice(1).join(" ")
		return cmd
	}


    /*
     *  Build the entire app into a single mod file. 
     */
    function buildFiles(out: String, files: Array) {
        rm(out)
        let cmd = getCompilerPath() + " --out " + out + " " + files.join(" ")
        trace("[BUILD]", cmd)
        let results = command(cmd)
        if (!exists(out)) {
            throw "Compilation failed for " + out + "\n" + results
        }
    }


    function buildApp(): Void {
        let pat = /\.es$/
        buildFiles("App.mod", glob("config", pat) + glob("src", pat) + glob("models", pat) + glob("controllers", /Base.es$/))
    }


    /*
     *  Build a single file. Used for controllers and models.
     */
    function build(files: String) {
        let out = files.replace(/.es$/,"") + ".mod"
        rm(out)

        let cmd: String
        if (appName) {
            cmd = getCompilerPath() + " --out " + out + " App.mod " + files
        } else {
            cmd = getCompilerPath() + " --out " + out + " " + files
        }
        trace("[BUILD]", cmd)
        let results = command(cmd)
        if (!exists(out)) {
            throw "Compilation failed for " + out + "\n" + results
        }
    }


    function browse(args: Array): Void {
        throw("No yet supported")
        let cmd = config.app.webserver

		if (!cmd.match(/^\/|^[a-zA-Z]:\//)) {
			cmd = cmd.replace(/^[^ ]+/, App.dir + "/" + "$&")
		}
		trace("[RUN]", cmd)
        System.run(cmd)
    }

    function deploy(args: Array): Void {
    }


    function install(args: Array): Void {
    }

    function generate(args: Array): Void {
        if (args.length == 0) {
            args.append("all")
        }

        let kind: String = args[0].toLower()
        let rest: Array = args.slice(1)

        if (rest.length == 0) {
            usage()
            return
        }

        switch (kind) {
        case "app":
            generateApp(rest)
            break

        case "controller":
            generateController(rest)
            break

        case "migration":
            generateMigration(rest)
            break

        case "model":
            generateModel(rest, "Create Model " + rest[0].toPascal())
            break

        case "scaffold":
            generateScaffold(rest)
            break

        default:
            usage()
            return
        }
    }


    /*
     *  ejsweb migrate              # Apply all migrations
     *  ejsweb migrate NNN          # Intelliegently set to a specific migration
     *  ejsweb migrate forward      # Migrate forward one
     *  ejsweb migrate backward     # Migrate backward one
     */
    function migrate(args: Array = null): Void {
        let files = File("db/migrations").getFiles().sort()
        let onlyOne = false
        let backward = false
        let targetSeq = null
        let id = null

        /*
         *  load("App.mod") - to load the models
         *  load("models/User.es")
         *      - Namespace environment is not available inside load()
         *          - Model needs use ejs.db
         *      - Database connection 
         *          
         */
        if (overwrite) {
            rm(dbPath)
            generateDatabase()
        }
        let migrations = _EjsMigration.findAll()
        let lastMigration = migrations.slice(-1)

        if (args && args.length > 0) {
            cmd = args.slice(0).toString().toLower()
        } else {
            cmd = ""
        }
        if (cmd == "forward" || cmd == "forw") {
            onlyOne = true

        } else if (cmd == "backward" || cmd == "back") {
            onlyOne = true
            backward = true

        } else if (cmd != "") {
            /* cmd may be a pure sequence number or a filename */
            targetSeq = cmd;
            let found = false
            for each (f in files) {
                let base = basename(f).toLower()
                if (basename(targetSeq) == base) {
                    targetSeq = base.replace(/^([0-9]*)_.*es/, "$1")
                    found = true
                } else {
                    let seq = base.replace(/^([0-9]*)_.*es/, "$1")
                    if (seq == targetSeq) {
                        found = true
                    }
                }
            }
            if (! found) {
                throw "Can't find target migration: " + targetSeq
            }
            if (lastMigration && targetSeq < lastMigration[0].version) {
                backward = true
            }
        }

        if (backward) {
            files = files.reverse()
        }

        for each (f in files) {
            let base = basename(f)
            if (!base.match(/^([0-9]+).*es/)) {
                continue
            }
            let seq = base.replace(/^([0-9]*)_.*es/, "$1")
            if (seq == "") {
                continue
            }
            let found = false
            for each (appliedMigration in migrations) {
                if (appliedMigration["version"] == seq) {
                    found = true
                    id = appliedMigration["id"]
                }
            }
            if (backward) {
                found = !found
                if (targetSeq && targetSeq == seq) {
                    return
                }
            }

            if (!found) {
                try { delete Migration; } catch {}
                load(f)
                if (backward) {
                    trace("[MIGRATE]", "Reverse " + base)
                    new Migration().backward(db)
                } else {
                    trace("[MIGRATE]", "Apply " + base)
                    new Migration().forward(db)
                }
                if (backward) {
                    _EjsMigration.remove(id)
                } else {
                    migration = new _EjsMigration
                    migration["version"] = seq.toString()
                    migration.save()
                }
                if (onlyOne) {
                    return
                }
            }
            if (!backward && targetSeq && targetSeq == seq) {
                return
            }
        }
        if (onlyOne) {
            if (backward) {
                trace("[OMIT]", "All migrations reversed")
            } else {
                trace("[OMIT]", "All migrations applied")
            }
        }
    }


    function run(args: Array): Void {
        let cmd = config.app.webserver

		if (!cmd.trim('"').match(/^\/|^[a-zA-Z]:\//)) {
			cmd = cmd.replace(/^[^ ]+/, App.dir + "/" + "$&")
		}
		trace("[RUN]", cmd)
        System.runx(cmd)
    }


    /*
     *  Generate an application.
     *
     *  ejsweb generate app appName
     */
    function generateApp(args: Array): Void {

        appName = args[0].toLower()
        let f: File = new File(appName)

        makeDir(appName)
        App.workingDir = appName
        makeDir(".tmp")
        makeDir(".ejs")
        makeDir("bin")
        makeDir("config")
        makeDir("controllers")
        makeDir("db")
        makeDir("db/migrations")
        makeDir("doc")
        makeDir("logs")
        makeDir("models")
        makeDir("messages")
        makeDir("test")
        makeDir("src")
        makeDir("utils")
        makeDir("views")
        makeDir("views/layouts")
        makeDir("web")
        makeDir("web/default")
        makeDir("web/images")
        makeDir("web/themes")

        generateConfig()
        generateLayouts()
        generatePages()
        generateBaseController()
        // generateStyles()
        generateReadme()
        generateDatabase()

        buildFiles("App.mod", ["controllers/Base.es"])
        App.workingDir = ".."

        if (verbose) {
			print("\nChange directory into your application directory: " + appName)
			print("Then run the web server via: \"" + ejsweb + " run\"")
            print("and point your browser at: http://localhost:4000/ to view your app.")
		}
    }


    function generateConfig(): Void {
        let data = Templates.Config.replace(/\${NAME}/g, appName)
        data = data.replace(/\${PATH}/g, App.workingDir)
        if (Config.Product == "ejs") {
            data = data.replace(/\${WEBSERVER}/g, "ejswebserver")
        } else {
            data = data.replace(/\${WEBSERVER}/g, "appweb")
        }
        if (App.dir == Config.BinDir) {
            /* Running installed */
            data = data.replace(/\${BINDIR}/g, Config.BinDir)
            data = data.replace(/\${HOME}/g, Config.LibDir)
        } else {
            /* Running out of a local dev tree */
            data = data.replace(/\${BINDIR}/g, App.dir)
			if (Config.OS == "WIN") {
				data = data.replace(/\${HOME}/g, dirname(App.dir) + "/bin")
			} else {
				data = data.replace(/\${HOME}/g, dirname(App.dir) + "/lib")
			}
        }
        makeConfigFile("config/config.ecf", data)
        makeConfigFile("config/compiler.ecf", Templates.Compiler.replace(/\${COMPILER}/g, compiler))
        makeConfigFile("config/database.ecf", Templates.Database)
        makeConfigFile("config/view.ecf", Templates.View)
    }


    /*
    function generateStyles(): Void {
        makeFile("web/themes/default.css", Templates.DefaultTheme, "Theme")
        makeFile("web/layout.css", Templates.DefaultLayoutStyle, "Layout Style")
    }
    */


    function generateLayouts(): Void {
        let data = Templates.DefaultLayout.replace(/\${NAME}/g, appName.toPascal())
        makeFile("views/layouts/default.ejs", data, "Layout")
    }


    function generatePages(): Void {
        if (exists(Config.LibDir + "/default-web/favicon.ico")) {
            path = Config.LibDir + "/default-web"
        } else {
            path = File(App.dir).dirname + "/lib/default-web"
        }
        for each (f in glob(path, /.*/)) {
            copyFile(f, "web" + f.slice(path.length), "Web File")
        }
    }


    function generateBaseController(): Void {
        let path = "controllers/Base.es"
        let data = Templates.BaseController.replace(/\${NAME}/g, appName)
        makeFile(path, data, "BaseController")
    }


    function generateReadme(): Void {
        let data: String = Templates.Readme.replace(/\${NAME}/g, appName.toPascal())
        makeFile("README", data, "README")
    }


    //  TODO - should this be here?
    function generateDatabase(): Void {
        db = new Database("db/" + appName + ".sdb")
        if (debug) {
            Database.trace(true)
        }
        db.createTable("_EjsMigrations", ["version:string"])
        //  TODO - UGLY
        _EjsMigration.setup()
    }


    /*
     *  ejsweb generate controller name [action ...]
     */
    function generateController(args: Array): Void {
        let name: String = args[0].toPascal()
        let actions = args.slice(1)
        let path: String = "controllers/" + name + ".es"
        let data: String = Templates.Controller.replace(/\${NAME}/g, name)
        data = data.replace(/\${APP}/g, appName)

        if (actions.length == 0) {
            actions.append("index")
        }
        for each (action in actions) {
            let actionData = Templates.Action.replace(/\${NAME}/g, action)
            data = data.replace(/NEXT_ACTION/, actionData + "NEXT_ACTION")
        }
        data = data.replace(/NEXT_ACTION/, "")
        makeFile(path, data, "Controller")
    }


    function createMigrationCode(model: String, forward: String, backward: String, comment: String) {
        data = Templates.Migration
        data = data.replace(/\${COMMENT}/g, comment)
        data = data.replace(/\${FORWARD}/g, forward)
        data = data.replace(/\${BACKWARD}/g, backward)

        seq = (new Date()).format("%Y%m%d%H%M%S")
        fileComment = comment.replace(/[ 	]+/g, "_")
        path = "db/migrations/" + seq + "_" + fileComment + ".es"
        if (exists(path)) {
            throw "Migration " + path + " already exists. Try again later."
        }
        makeFile(path, data, "Migration")
    }


    function validateAttributes(attributes: Array): Void {
        for each (attribute in attributes) {
            column = attribute.split(":")[0]
            datatype = attribute.split(":")[1]
            if (!Database.DatatypeToSqlite[datatype]) {
                throw "Unsupported data type: " + datatype + " for column " + column
            }
        }
    }

    function createMigration(model: String, attributes: Array, comment: String, tableExists: Boolean): Void {

        let tableName = plural(model).toPascal();

        let forward = ''
        let backward = ''

        if (attributes && attributes.length > 0) {
            validateAttributes(attributes)
            if (!tableExists) {
                forward = '        db.createTable("' + tableName + '", ["' + attributes.join('", "') + '"])'
                backward = '        db.destroyTable("' + tableName + '")'

            } else {
                forward = ""
                for each (col in attributes)  {
                    spec = col.split(":")
                    forward += '        db.addColumn("' + tableName + '", "' + spec[0] + '", "' + spec[1] + '")\n'
                }
                backward = '        db.removeColumns("' + tableName + '", ['
                for each (col in attributes) {
                    backward += '"' + col.split(":")[0] + '", '
                }
                backward += '])'
            }

        } else {
            if (reverse) {
                forward = '        db.destroyTable("' + tableName + '")'
            }
        }
        if (reverse) {
            createMigrationCode(model, backward, forward, comment)
        } else {
            createMigrationCode(model, forward, backward, comment)
        }
    }


    /*
     *  ejsweb generate migration description model [field:type ...]
     */
    function generateMigration(args: Array): Void {
        if (args.length < 2) {
            usage()
        }
        comment = args[0]
        model = args[1]
        createMigration(model, args.slice(2), comment, true)
    }


    /*
     *  ejsweb generate model name [field:type ...]
     */
    function generateModel(args: Array, comment: String): Void {
        let model: String = args[0].toPascal()
        if (model.endsWith("s")) {
            eprint("WARNING: Models should typically be singluar not plural. Continuing ...")
        }
        let path = "models/" + model + ".es"

        if (exists(path) && !overwrite) {
            traceFile(path, "[EXISTS] Migration (model already exists)")
        } else {
            createMigration(model, args.slice(1), comment, false)
        }

        let data = Templates.Model.replace(/\${NAME}/g, model)
        makeFile(path, data, "Model")
    }


    /*
     *  ejsweb generate scaffold model [field:type ...]
     */
    function generateScaffold(args: Array): Void {
        let model = args[0]
        let controller = model.toPascal()
        let attributes = args.slice(2)

        makeDir("views/" + controller)
        generateModel(args, "Create Scaffold " + model)
        generateScaffoldController(controller, model)
        generateScaffoldViews(controller, model)
        buildApp()
        if (!applyMigration && !quiet) {
			print("\nDon't forget to apply the database migration. Run: \"" + ejsweb + " migrate\"")
		}
    }


    /*
     *  Create a controller with scaffolding. Usage: controllerName [actions ...]
     */
    function generateScaffoldController(controller: String, model: String, extraActions: Array = null): Void {
        let name = controller.toPascal()
        let path = "controllers/" + name + ".es"

        let stndActions: Array = [ "index", "list", "create", "edit", "update", "destroy" ]
        let views: Array = [ "list", "edit" ]
        let actions: Array = []

        if (extraActions) {
            for each (action in extraActions) {
                if (! stndActions.contains(action)) {
                    actions.append(action.toCamel())
                }
            }
        }

        let data: String = Templates.ScaffoldController.replace(/\${APP}/g, appName.toPascal())
        data = data.replace(/\${NAME}/g, name)
        data = data.replace(/\${MODEL}/g, model.toPascal())
        data = data.replace(/\${LOWER_MODEL}/g, model.toLower())

        for each (action in actions) {
            let actionData = Templates.Action.replace(/\${NAME}/g, action)
            data = data.replace(/NEXT_ACTION/, actionData + "NEXT_ACTION")
        }
        data = data.replace(/NEXT_ACTION/, "")

        makeFile(path, data, "Controller")
    }


    /*
     *  Create a scaffold views.  Usage: controllerName [actions ...]
     */
    function generateScaffoldViews(controller: String, model: String, extraActions: Array = null): Void {

        let stndActions: Array = [ "index", "list", "create", "edit", "update", "destroy" ]
        let views: Array = [ "list", "edit" ]
        let actions: Array = stndActions.clone()

        if (extraActions) {
            for each (action in extraActions) {
                if (! stndActions.contains(action)) {
                    views.append(action.toCamel())
                }
            }
        }
        let data: String

        model = model.toPascal()

        for each (view in views) {
            switch (view) {
            case "edit":
                data = Templates.ScaffoldEditView.replace(/\${MODEL}/g, model)
                data = data.replace(/\${LOWER_MODEL}/g, model.toLower())
                break
            case "list":
                data = Templates.ScaffoldListView.replace(/\${MODEL}/g, model)
                break
            default:
                data = Templates.ScaffoldView.replace(/\${MODEL}/g, model)
                data = data.replace(/\${LOWER_MODEL}/g, model.toLower())
                data = data.replace(/\${CONTROLLER}/g, controller)
                data = data.replace(/\${VIEW}/g, view)
                break
            }
            let path: String = "views/" + controller + "/" + view + ".ejs"
            makeFile(path, data, "View")
        }
    }


    function checkApp(cmd: String, rest: Array): Void {
        if (cmd == "generate") {
            let what = rest[0]
            if (rest[0] == "app") {
                return
            }
            if (what != "app" && what != "controller" && what != "migration" && what != "model" && what != "scaffold") {
                usage()
                App.exit()
            }
        }
        let dirs: Array = [ "config", "controllers", "views"  ]
        for each (d in dirs) {
            if (! isDir(d)) {
                if (cmd == "compile") {
                    return
                }
                throw "Can't find \"" + d + "\" directory. Run from inside the application directory"
            }
        }

        let files: Array = [ "config/compiler.ecf", "config/config.ecf", "config/database.ecf", "config/view.ecf" ]
        for each (f in files) {
            if (! exists(f)) {
                throw "Can't find \"" + f + "\" Run from inside the application directory\n" +
                      "Use " + ejsweb + " generate app NAME to create a new Ejscript web application"
            }
        }
        appName = basename(App.workingDir).toLower()
        dbPath = "db/" + appName + ".sdb"

        loadConfigFile("config/compiler.ecf", "compiler", false)

        if (!exists(dbPath)) {
            generateDatabase()
        }

        db = Database.defaultDatabase = new Database("db/" + appName + ".sdb")
        if (debug) {
            Database.trace(true)
            _EjsMigration.trace(true)
        }
        //  TODO - this is ugly. Need to put here rather than in model because we need to have the database setup first.
        _EjsMigration.setup(db)
    }


    function loadConfigFile(file: String, objName: String, mandatory: Boolean = false): Boolean {
        let settings: Object = loadEcf(file, mandatory)
        if (settings == null) {
            return false
        }
        let obj = config[objName] = {}
        for (key in settings) {
            obj[key] = settings[key]
        }
        return true
    }


    function loadEcf(path: String, mandatory: Boolean = false): Object {
        if (!exists(path)) {
            if (mandatory) {
                throw new IOError("Can't open required configuration file: " + path)
            } else {
                return null
            }
        }
        try {
            let data = "{ " + File.getString(path) + " }"
            return deserialize(data)
        } catch (e: Error) {
            throw new IOError("Can't load " + path + e)
        }
    }


    /*
     *  Make an ECF file that lives under ./config
     */
    function makeConfigFile(path: String, data: String): Void {
        if (exists(path) && !overwrite) {
            return
        }
        data = data.replace(/\${NAME}/g, appName)
        makeFile(path, data, "Config File")
    }


    function makeFile(path: String, data: String, msg: String): Void {

        let f: File = new File(path)
        if (f.exists && !overwrite) {
            traceFile(path, "[EXISTS] " + msg)
            return
        }

        if (! f.exists) {
            traceFile(path, "[CREATED] " + msg)
        } else {
            traceFile(path, "[OVERWRITTEN] " + msg)
        }

        f.open(File.Write | File.Create | File.Truncate)
        f.write(data)
        f.close()
    }


    function makeDir(path: String): Void {
        if (isDir(path)) {
            return
        }
        trace("[CREATED] " + "Directory", path)
        mkdir(path, DIR_PERMS)
    }


    function copyFile(from: String, to: String, msg: String) {

        let f: File = new File(to)
        if (f.exists && !overwrite) {
            traceFile(to, "[EXISTS] " + msg)
            return
        }

        if (! f.exists) {
            traceFile(to, "[CREATED] " + msg)
        } else {
            traceFile(to, "[OVERWRITTEN] " + msg)
        }
        makeDir(File(to).dirname)
        cp(from, to)
    }


    /*
     *  Find all files matching the pattern 
     */
    function glob(path: String, pattern: RegExp, recurse: Boolean = true): Array {
        let result: Array = new Array
        if (isDir(path)) {
            if (recurse) {
                for each (f in ls(path, true)) {
                    let got: Array = glob(f, pattern)
                    for each (i in got) {
                        result.append(i)
                    }
                }
            }

        } else {
            if (path.match(pattern)) {
                result.append(path)
            }
        }
        return result
    }


    function globSubdirs(path: String): Array {
        let result: Array = new Array
        for each (f in ls(path, true)) {
            if (isDir(f)) {
                result.append(f)
            }
        }
        return result
    }


    function getNthSegment(path: String, nth: Number) {
        let segments: Array = path.split(/(\\|\/)+/g)
        for (let i: Number = segments.length - 1; i >= 0; i--) {
            if (segments[0] == ".") {
                segments.remove(i, i)
            }
        }
        return segments[nth]
    }


    function command(cmd: String): String {
        let results
        try {
            results = System.run(cmd)
        } 
        catch (e) {
            msg = "Compilation failure, for " + cmd + "\n\n" +
                e.toString().replace(/Error Exception: Command failed: Status code [0-9]*.\n/, "")
            throw msg
        }
        return results
    }


    /*
    function error(...args): Void {
        //  TODO - need a better way to write error messages without the "ejs: Error: String: ejs" prefix
        throw(ejsweb + ": " + args.join(" "))
        App.exit(1)
    }
    */


    function traceFile(path: String, msg: String): Void {
        //  TODO - string method to add quotes would be useful
        trace(msg, '"' + path + '"')
    }


    function trace(tag: String, ...args): Void {
        if (verbose) {
            print("  " + tag + ": " + args.join(" "))
        }
    }


    //  TODO - share with rest of framework
    function plural(word: String): String {
        return word + "s"
    }


    function singular(word: String) {
        //  TODO
    }
}


dynamic class _EjsMigration implements Record {
    setup()

    function _EjsMigration(fields: Object = null) {
        constructor(fields)
    }
}


/*
 *  Templates for various files
 */
class Templates {
    
    /*
     ***************** config/config.ecf template ***********************
     */
    public static const Config =
'
app: {
    mode: "debug",
    webserver: \'"${BINDIR}/${WEBSERVER}" --home "${HOME}" --ejs "/:${PATH}/" --log stdout:2\'
},
'


    /*
     ***************** config/compiler.ecf template ***********************
     */
    public static const Compiler = 
"
debug: {
    command: '${COMPILER} --lang fixed --debug --optimize 9 --web ',
},

test: {
    command: '${COMPILER} --lang fixed --debug --optimize 9 --web ',
},

production: {
    command: '${COMPILER} --lang fixed --optimize 9 --web ',
},
"


    /*
     ***************** config/database.ecf template ***********************
     */
    public static const Database = 
'
debug: {
    adapter: "sqlite3",
    database: "db/${NAME}.sdb",
    username: "",
    password: "",
    timeout: 5000,
},

test: {
    adapter: "sqlite3",
    database: "db/${NAME}.sdb",
    username: "",
    password: "",
    timeout: 5000,
},

production: {
    adapter: "sqlite3",
    database: "db/${NAME}.sdb",
    username: "",
    password: "",
    timeout: 5000,
},
'


    /*
     ***************** config/view.ecf template ***********************
     */
    public static const View = 
'
connectors: {
    table: "html",
    chart: "google",
    rest: "html",
},

'


    /*
     *****************  BaseController template ***********************
     */
    public static const BaseController = 
'/*
 *  BaseController.es - Base class for all controllers
 */

public class BaseController extends Controller {

    public var title: String = "${NAME}"
    public var style: String

    /*
     *  Turn on SQL statement logging
     *      Record.trace(true)
     */
    function BaseController() {
        style = appUrl + "/web/style.css"
    }
}
'


    /*
     *****************  Controller template ***********************
     */
    public static const Controller = 
'
public class ${NAME}Controller extends BaseController {

    function ${NAME}Controller() {
    }

    NEXT_ACTION
}
'


    /*
     *****************  ScaffoldController template ******************
     */
    public static const ScaffoldController = 
'
public class ${NAME}Controller extends BaseController {

    function ${NAME}Controller() {
    }

    public var ${LOWER_MODEL}: ${MODEL}

    use namespace action

    action function index() { 
        renderView("list")
    }

    action function list() { 
    }

    action function edit() {
        ${LOWER_MODEL} = ${MODEL}.find(params.id)
    }

    action function create() {
        ${LOWER_MODEL} = new ${MODEL}
        renderView("edit")
    }

    action function update() {
        if (params.commit == "Cancel") {
            redirect("list")

        } else if (params.commit == "Delete") {
            destroy()

        } else if (params.id) {
            ${LOWER_MODEL} = ${MODEL}.find(params.id)
            if (${LOWER_MODEL}.saveUpdate(params.${LOWER_MODEL})) {
                inform("${MODEL} updated successfully.")
                redirect("list")
            } else {
                /* Validation failed */
                renderView("edit")
            }

        } else {
            ${LOWER_MODEL} = new ${MODEL}(params.${LOWER_MODEL})
            if (${LOWER_MODEL}.save()) {
                inform("New ${LOWER_MODEL} created")
                redirect("list")
            } else {
                renderView("edit")
            }
        }
    }

    action function destroy() {
        ${MODEL}.remove(params.id)
        inform("${MODEL} " + params.id + " removed")
        redirect("list")
    }

    NEXT_ACTION
}
'


    /*
     *****************  ScaffoldListView template ******************
     */
    public static const ScaffoldListView = 
'<h1>${MODEL} List</h1>

<% table(${MODEL}.findAll(), {click: "edit"}) %>
<br/>
<% buttonLink("New ${MODEL}", "create") %>
'


    /*
     *****************  ScaffoldEditView template ******************
     */
    public static const ScaffoldEditView = 
'<h1><%= (${LOWER_MODEL}.id) ? "Edit" : "Create" %> ${MODEL}</h1>

<% form("update", ${LOWER_MODEL}) %>

    <table border="0">
    <% for each (name in ${MODEL}.columnNames) {
        if (name == "id") continue
        uname = name.toPascal()
    %>
        <tr><td>@@uname</td><td><% input(name) %></td></tr>
    <% } %>
    </table>

    <% button("OK", "commit") %>
    <% button("Cancel", "commit") %>
    <% if (${LOWER_MODEL}.id) button("Delete", "commit") %>
<% endform() %>
'


    /*
     *****************  ScaffoldView template ******************
     */
    public static const ScaffoldView = 
'<h1>View "${CONTROLLER}/${VIEW}" for Model ${MODEL}</h1>
<p>Edit in "views/${CONTROLLER}/${VIEW}.ejs"</p>
'


    /*
     ***********************  Action template ***********************
     */
    public static const Action = '
    action function ${NAME}() {
    }

'


    /*
     ***********************  Model template ***********************
     */
    public static const Model = 
'/*
 *  ${NAME}.es - ${NAME} Model Class
 */

public dynamic class ${NAME} implements Record {

    setup()

    function ${NAME}(fields: Object = null) {
        constructor(fields)
    }
}
'


    /*
     ***********************  Migration template ***********************
     */
    public static const Migration = 
'/*
 *  ${COMMENT}
 */
public class Migration {

    function forward(db) {
${FORWARD}    }

    function backward(db) {
${BACKWARD}
    }
}
'



    /*
     **************************** README template ***********************
     */
    public static const Readme = 
'
README - Overview of files and documentation generated by ejsweb

These Directories are created via "ejsweb generate ${NAME}:"

    bin                       Programs and scripts
    config                    Configuration files
    controllers               Controller source
    db                        SQL databases and database scripts
    db/migrations             SQL database migration scripts
    doc                       Documentation for the application
    logs                      Log files
    messages                  Internationalization messages
    models                    Database models
    src                       Extra application source
    test                      Test files
    views                     View source files
    views/layouts             View layout files
    web                       Public web directory
    web/themes                Theme style sheet directory
    .ejs                      State files used by ejsweb
    .tmp                      Temporary files

These files are also created:

    config/compiler.ecf       Compiler options
    config/config.ecf         General application configuration 
    config/database.ecf       Database connector configuration 
    config/view.ecf           View connector configuration 
    views/layouts/default.ejs Default template page for all views
    web/layout.css            Default layout style sheet
    web/themes/default.css    Default theme style sheet
    web/images/banner.jpg     Default UI banner
'


    /*
     ***************************  View header and footer templates ******************
     */
    public static const ViewHeader = 
'

public dynamic class ${CONTROLLER}${VIEW}View extends View {
    function ${CONTROLLER}${VIEW}View(c: Controller) {
        super(c)
    }

    override public function render() {
'

    public static const ViewFooter = '
    }
}
'

    /*
     ***************************  Default Layout templates ******************
     */
    public static const DefaultLayout = 
'<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
    <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
    <title>@@title</title>
    <% stylesheet(["web/layout.css", "web/themes/default.css"]); %>
    <% script("web/js/jquery.js") %>
</head>

<body>
    <div class="top">
        <h1><a href="@@appUrl/">${NAME} Application</a></h1>
    </div>

    <% flash(["inform", "error", "message", "warning"]) %>
    <div class="content">
        <%@ content %>
    </div>

    <div class="bottom">
        <p class="footnote">Powered by Ejscript&trade;</p>
    </div>
</body>
</html>
'


/* End of class Templates */
}


/*
 *  Ejsweb Parser - Parse an ejs file and emit a Ejscript compiled version
 *
 *  This parser handles embedded Ejscript using <% %> directives. It supports:
 *
 *    <%                    Begin an ejs directive section containing statements
 *    <%=                   Begin an ejs directive section that contains an expression to evaluate and substitute
 *    %>                    End an ejs directive
 *    <%@ include "file" %> Include an ejs file
 *    <%@ layout "file" %>  Specify a layout page to use. Use layout "" to disable layout management.
 *
 *  Directives for use outside of <% %> 
 *    @@var                 To expand the value of "var". Var can also be simple expressions (without spaces).
 *
 *  TODO implement these directives
 *    -%>                   Omit newline after tag
 */

/*
 *  Parser tokens
 */
class Token {
    public static const Err			= -1		/* Any input error */
    public static const Eof			= 0			/* End of file */
    public static const EjsTag  	= 1			/* <% text %> */
    public static const Var 		= 2			/* @@var */
    public static const Literal		= 3			/* literal HTML */
    public static const Equals		= 4			/* <%= expression */
    public static const Control 	= 6			/* <%@ control */

    public static var tokens = [ "Err", "Eof", "EjsTag", "Var", "Literal", "Equals", "Control" ]
}


class EjsParser {

    private const ContentMarker: String         = "__ejs:CONTENT:ejs__"
    private const ContentPattern: RegExp        = new RegExp(ContentMarker)
    private const LayoutsDir: String            = "views/layouts"

    private var appBaseDir: String
    private var script: String
    private var pos: Number                     = 0
    private var lineNumber: Number              = 0
    private var layoutPage: String


    /*
     *  Main parser. Parse the script and return the compiled (Ejscript) result
     */
    public function parse(file: String, appDir: String, layout: string): String {

        var token: ByteArray = new ByteArray
        var out: ByteArray = new ByteArray
        var tid: Number

        appBaseDir = appDir;
        layoutPage = layout
        script = File.getString(file)

        while ((tid = getToken(token)) != Token.Eof) {

            // print("getToken => " + Token.tokens[tid + 1] + " TOKEN => \"" + token + "\"")

            switch (tid) {
            case Token.Literal:
                out.write("\nwrite(\"" + token + "\");\n")
                break

            case Token.Var:
                /*
                 *	Trick to get undefined variables to evaluate to "".
                 *	Catenate with "" to cause toString to run.
                 */
                out.write("\nwrite(\"\" + ", token, ");\n")
                break

            case Token.Equals:
                out.write("\nwrite(\"\" + (", token, "));\n")
                break

            case Token.EjsTag:
                /*
                 *  Just copy the Ejscript code straight through
                 */
                //  TODO BUG ByteArray.write(ByteArray) is not working. Requires toString()
                out.write(token.toString())
                break

            case Token.Control:
                let args: Array = token.toString().split(/\s/g)
                let cmd: String = args[0]

                switch (cmd) {
                case "include":
                    let path = args[1].trim("'").trim('"')
                    let incPath = (path[0] == '/') ? path: dirname(file) + "/" + path
                    /*
                     *	Recurse and process the include script
                     */
                    let inc: EjsParser = new EjsParser
                    out.write(inc.parse(incPath, appBaseDir, undefined))
                    break

                case "layout":
                    let path = args[1]
                    if (path == "" || path == '""') {
                        layoutPage = undefined
                    } else {
                        path = args[1].trim("'").trim('"').trim('.ejs') + ".ejs"
                        layoutPage = (path[0] == '/') ? path : (LayoutsDir + "/" + path)
                        if (! exists(layoutPage)) {
                            error("Can't find layout page " + layoutPage)
                        }
                    }
                    break

                case "content":
                    out.write(ContentMarker)
                    break

                default:
                    error("Bad control directive: " + cmd)
                }
                break

            default:
            case Token.Err:
                //  TODO - should report line numbers
                error("Bad input token: " + token)

            }
        }

        if (layoutPage != undefined && layoutPage != file) {
            let layoutText: String = new EjsParser().parse(layoutPage, appBaseDir, layoutPage)
            return layoutText.replace(ContentPattern, out.toString())
        }
        return out.toString()
    }


    /*
     *  Get the next input token. Read from script[pos]. Return the next token ID and update the token byte array
     */
    function getToken(token: ByteArray): Number {

        var tid = Token.Literal

        token.flush()

        var c
        while (pos < script.length) {
            c = script[pos++]

            switch (c) {

            case '<':
                if (script[pos] == '%' && (pos < 2 || script[pos - 2] != '\\')) {
                    if (token.available > 0) {
                        pos--
                        return Token.Literal
                    }
                    pos++
                    eatSpace()
                    if (script[pos] == '=') {
                        /*
                         *  <%=  directive
                         */
                        pos++
                        eatSpace()
                        while ((c = script[pos]) != undefined && 
                                (c != '%' || script[pos+1] != '>' || script[pos-1] == '\\')) {
                            token.write(c)
                            pos++
                        }
                        pos += 2
                        return Token.Equals

                    } else if (script[pos] == '@') {
                        /*
                         *  <%@  directive
                         */
                        pos++
                        eatSpace()
                        while ((c = script[pos]) != undefined && (c != '%' || script[pos+1] != '>')) {
                            token.write(c)
                            pos++
                        }
                        pos += 2
                        return Token.Control

                    } else {
                        while ((c = script[pos]) != undefined && 
                                (c != '%' || script[pos+1] != '>' || script[pos-1] == '\\')) {
                            token.write(c)
                            pos++
                        }
                        pos += 2
                        return Token.EjsTag
                    }
                }
                token.write(c)
                break

            case '@':
                if (script[pos] == '@' && (pos < 1 || script[pos-1] != '\\')) {
                    if (token.available > 0) {
                        pos--
                        return Token.Literal
                    }
                    pos++
                    c = script[pos++]
                    while (c.isAlpha || c.isDigit || c == '[' || c == ']' || c == '.' || c == '$' || 
                            c == '_' || c == "'") {
                        token.write(c)
                        c = script[pos++]
                    }
                    pos--
                    return Token.Var
                }
                token.write(c)
                break

            case "\r":
            case "\n":
                lineNumber++
                token.write(c)
                tid = Token.Literal
                break

            default:
                //  TODO - triple quotes would eliminate the need for this
                if (c == '\"' || c == '\\') {
                    token.write('\\')
                }
                token.write(c)
                break
            }
        }
        if (token.available == 0 && pos >= script.length) {
            return Token.Eof
        }
        return tid
    }


    function eatSpace(): Void {
        while (script[pos].isSpace) {
            pos++
        }
    }


    function error(msg: String): Void {
        throw "ejsgen: " + msg + ". At line " + lineNumber
    }
}

/*
 *  Main program
 */
var eweb: EjsWeb = new EjsWeb
eweb.loadDefaults()

try {
    if (!eweb.parseArgs(App.args)) {
        eweb.usage()
    }
}
catch (e) {
    eprint("ejsweb: " + e)
    App.exit(2)
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
