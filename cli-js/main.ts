#!/usr/bin/env node

/*

    CopyLeft 2023 maddsua
    https://github.com/maddsua/svgbundler
    License - No license (specified MIT but not really)
    Use it however you want
    I don't guarantee anything, but at very least, this package is safe from dependency-related security issues

*/

import fs from 'fs';

import { separatePath, normalizePath } from '@maddsua/webappkit/textutils';


const minSvgLength = 32;
const fsWatch_evHold = 100;
const maxSearchDepth = 32;
const styleSourceFile = /\.s?css$/;


/*
	Console stuff
*/
const printTime = () => {
	console.log(`[${new Date().toLocaleTimeString()}]`);
};
const colorText = (text: string, color: string | null, style: string | null) => {
	const table = {
		black: '\x1b[30m',
		red: '\x1b[31m',
		green: '\x1b[32m',
		yellow: '\x1b[33m',
		blue: '\x1b[34m',
		magenta: '\x1b[35m',
		cyan: '\x1b[36m',
		white: '\x1b[37m'
	};
	const styles = {
		bright: '\x1b[1m',
		dim: '\x1b[2m',
		underscore: '\x1b[4m',
		blink: '\x1b[5m',
		reverse: '\x1b[7m',
		hidden: '\x1b[8m'
	};

	return (table[color] || table.white) + (styles[style] || '') + text + '\x1b[0m';
};


/*
	Flags and other stuff
*/
const flags = {
	optimize: false,
	watch: false,
	silent: false,
	recursive: false,
	flatten: false,
	loadFromPackage: false
};
let classPrefixText = '';
const pushPreClass = (arg:string, argpatt:string) => {
	if (!arg.startsWith(argpatt)) return false;
	const matched = arg.slice(argpatt.length).match(/([A-Za-z\_\-]+[0-9\.]*)/g);
		if (!matched) return false;
	classPrefixText = matched.join('');
	return true;
};


/*
	Stuff to get input paths
*/
interface i_pathPair {
	from: string,
	to: string,
	override?: boolean,
	prefix?: string,
	selector?: string
};
const sources = {
	svg: Array<i_pathPair>(0),
	css: Array<i_pathPair>(0)
};


const parseInputsJSON = (jsonFilePath: string) => {

	try {

		const jsonDataFile = JSON.parse(new TextDecoder().decode(new Uint8Array(fs.readFileSync(jsonFilePath))));

		if (typeof jsonDataFile !== 'object') throw 'No files wtf bro';

		const bundlerBlock = ('svgbundler' in jsonDataFile) ? jsonDataFile['svgbundler'] : jsonDataFile;

		const bundlerFilesList = bundlerBlock['files'];

		bundlerFilesList.forEach((item: i_pathPair) => {
			//	perform checks
			if (typeof item.from !== 'string') return;
			if (typeof item.to !== 'string') return;
			if (typeof item.prefix !== 'string') item.prefix = null;
			if (typeof item.selector !== 'string') item.selector = null;
			if (typeof item.override !== 'boolean') item.override = false;
	
			//	decide where to put this Harry Potter
			(styleSourceFile.test(item.from) ? sources.css : sources.svg).push(item);
		});

		
	} catch (error) {
		console.log('No inputs loaded from', jsonFilePath);
		return true;
	}

	return true;
}


/*
	Process start arguments
*/
process.argv.forEach((arg) => {

	pushPreClass(arg, '--prefix=');

	if (arg === '--recursive' || arg === '-r') flags.recursive = true;
	if (arg === '--flatten' || arg === '-f') flags.flatten = true;
	if (arg === '--minify' || arg === '-m') flags.optimize = true;
	if (arg === '--watch' || arg === '-w') flags.watch = true;
	if (arg === '--flags.silent' || arg === '-s') flags.silent = true;
	if (arg === '--package' || arg === '-p') flags.loadFromPackage = true;

	//	add sources
	(() => {
		const validPath = /([\w\d\\/\.\-\_]{3,})/g;
			if (!(new RegExp(`^${validPath.source}\\:${validPath.source}$`,'g')).test(arg)) return;
		const match = arg.match(validPath);
			if (match?.length !== 2) return false;
		(match[0].match(styleSourceFile) ? sources.css : sources.svg).push({from: normalizePath(match[0]), to: normalizePath(match[1])});
	})();
});

//	if enabled, load sources from package.json
if (flags.loadFromPackage) parseInputsJSON('./package.json');

//	and by default try to get them from own config file
parseInputsJSON('./svgbundler.config.json');


/*
	Check if any files are added to queeu
*/
if (!(sources.css.length + sources.svg.length)) {
	console.error(colorText('Specify at least one input directory+destination file or a .css to bundle', 'red', null), '\r\n');
	console.error(colorText(' Build aborted ', 'red', 'reverse'));
	process.exit(1);
}


/*
	Notify that we will add prefixes
*/
if (classPrefixText.length) {
	console.log('Prefix', colorText(classPrefixText, 'yellow', null), 'will be added to all the classes\r\n');
}


/*
	Stuff to find files
*/
const findAllFiles = (inDirectory: string, format: string, recursive: boolean) => {

	let results: string[] = [];
	
	//	(0 - 1) so on the first run the nesting will be equal to zero
	let nested = recursive ? -1 : (maxSearchDepth + 1);

	const dir_search = (searchDir: string) => {	
		nested++;

		if (!fs.existsSync(searchDir)) {
			console.error(colorText(`Directory '${searchDir}' does not exist`, 'red', 'reverse'));
			return;
		}

		fs.readdirSync(searchDir).forEach((file) => {
			const filePath = `${searchDir}/${file}`;
			const stat = fs.lstatSync(filePath);
	
			if (stat.isDirectory() && nested < maxSearchDepth) dir_search(filePath); 
				else if (filePath.endsWith(`.${format}`)) results.push(filePath);
		});

		nested--;
	};
	dir_search(inDirectory);
	
	return results;
};


/*
	Stuff to minify files
*/
const minify = (xml:string) => {

	const regexes = {
		rmAttribs: ['xmlns:serif', 'serif:id'],
		patterns: [
			//	comments
			/\<\!\-\-.{0,}\-\-\>/g,
			
			//	empty <g> tags
			/<g[\w\s\"\'\=]*>[\n\s\t]*<\/g>/g
		]
	};

	//	remove tabs and newlines
	xml = xml.replace(/[\t\r\n]/g, ' ');

	//	remove attribs
	regexes.rmAttribs.forEach((attrib) => {
		//	/(attrib\=\"[^\"]{0,}\")|(attrib\=\'[^\']{0,}\')/g
		const domAttribRegex = new RegExp(`(${attrib}\\=\\\"[^\\\"]{0,}\\\")|(${attrib}\\=\\'[^\\']{0,}\\')`, 'g');
		if (domAttribRegex.test(xml)) xml = xml.replace(domAttribRegex, '');
	});
	//	remove patterns
	regexes.patterns.forEach((pattern) => {
		if (pattern.test(xml)) xml = xml.replace(pattern, '');
	});

	//	remove repeating whitespaces
	xml = xml.replace(/\s{2,}/g, ' ');

	//	remove whitespace between tags: .../> <...
	xml = xml.replace(/>\s</g, '><');

	return xml;
};


/*
	Stuff to do the job
*/
const bundle_svg = (svgDir:i_pathPair) => {
	console.log(colorText(`Compiling to ${svgDir.to} `, 'green', null));

	let writeContents = '';
	let imagesLoaded = 0;

	findAllFiles(svgDir.from, 'svg', flags.recursive).forEach((svgFile) => {
		let svgtext = '';
		try { svgtext = fs.readFileSync(svgFile, {encoding: 'utf8'}) }
		catch (error) {
			console.warn(colorText(' Cant\'t read file:', 'red', null), svgFile);
			return '';
		}

		let classname = (() => {
			let temp = svgFile.slice(svgDir.from.length);
				if (temp.includes('.')) temp = temp.slice(0, temp.indexOf('.'));
				if (temp.startsWith('/')) temp = temp.slice(1);
				if (flags.flatten) temp = temp.replace(/-/g, '_');

			let classPrefix = svgDir.prefix ? svgDir.prefix : classPrefixText;
				if (classPrefix.length) classPrefix += '-';
			return `.${classPrefix}${temp.replace(/[\/]{1,}|[\\]{1,}/g, '-')}`;
		})();

		if (flags.optimize) svgtext = minify(svgtext);

		if (svgtext.length < minSvgLength) {
			console.warn(colorText(` -! ${svgFile}`, 'yellow', null), '\r\n');
			return;
		}

		writeContents += svgDir.selector ? `${classname}:${svgDir.selector}` : `${classname}, ${classname}-after::after, ${classname}-before::before`;

		writeContents += ` {\r\n\tbackground-image: url(\"data:image/svg+xml,${encodeURIComponent(svgtext)}\")${svgDir.override ? '!important' : ''};\r\n}\r\n`;

		if (!flags.silent) console.log(` --> ${svgFile}`);
		imagesLoaded++;
	});

	if (writeContents.length < minSvgLength) {
		console.warn(colorText('No svgs bundled', 'yellow', null), '\r\n');
		return;
	}

	const destDir = separatePath(svgDir.to).dir;
		if (!fs.existsSync(destDir)) fs.mkdirSync(destDir, { recursive: true });
	fs.writeFileSync(svgDir.to, writeContents, {encoding: 'utf8'});

	if (flags.silent) console.log(colorText(`+ ${imagesLoaded} images`, 'green', null), '\r\n');
		else  console.log(colorText('...done', 'green', null), '\r\n');
};


/*
	Stuff to do the job a lil differently
*/
const bundle_css = (file: i_pathPair) => {
	console.log(colorText(`Bundling ${file.from} to ${file.to} `, 'green', null));

	let cssText = '';
	try { cssText = fs.readFileSync(file.from, {encoding: 'utf-8'}) }
	catch (error) { 
		console.warn(colorText(' Cannot read: ', 'yellow', 'reverse'), colorText(file.from, 'yellow', null));
		return;
	}

	const regexes = {
		bgimgOpen: /url\([\'\"]*/,
		bgimgClose: /[\'\"]*\)/,
		svgPath: /([\w\d\\\/\.\-\_]+\.svg)/,
		ascend: /^\.\./
	};

	let imagesLoaded = 0;
	cssText.match(new RegExp(regexes.bgimgOpen.source + regexes.svgPath.source + regexes.bgimgClose.source, 'g'))?.forEach((image) => {

		const providedPath = image.match(regexes.svgPath)[0];

		const fsReadPath = normalizePath(regexes.ascend.test(providedPath) ? (separatePath(file.from).dir + providedPath) : providedPath);

		let svgtext = '';

		try { 
			svgtext = fs.readFileSync(fsReadPath, {encoding: 'utf-8'});
		} catch (error) { 
			console.warn(colorText(` -! ${fsReadPath}`, 'yellow', null));
			return;
		}

		if (flags.optimize) svgtext = minify(svgtext);

		if (svgtext.length < minSvgLength) return;

		cssText = cssText.replace(image, `url("data:image/svg+xml,${encodeURIComponent(svgtext)}")`);
		
		if (!flags.silent) console.log(` --> ${fsReadPath}`);
		imagesLoaded++;
	});

	const destDir = separatePath(file.to).dir;
		if (!fs.existsSync(destDir)) fs.mkdirSync(destDir, { recursive: true });
	fs.writeFileSync(file.to, cssText, {encoding: 'utf8'});

	if (imagesLoaded) {
		if (flags.silent) console.log(colorText(`+ ${imagesLoaded} images`, 'green', null), '\r\n');
			else console.log(colorText('...done', 'green', null), '\r\n');
		
	} else console.log(colorText('...nothing to do here', 'yellow', null), '\r\n');
}


/*
	Add watchdogs for svg sources
*/
sources.svg.forEach((item) => {
	bundle_svg(item);
	if (flags.watch) {
		let changeHandler: NodeJS.Timeout | number = 0;
		if (!fs.existsSync(item.from)) return;
		fs.watch(item.from, () => {
			clearTimeout(changeHandler);
			changeHandler = setTimeout(() => {
				printTime();
				bundle_svg(item);
			}, fsWatch_evHold);
		});
	}
});


/*
	Add watchdogs for css sources
*/
sources.css.forEach((item) => {
	bundle_css(item);
	if (flags.watch) {
		let locked = false;
		let changeHandler: NodeJS.Timeout | number = 0;
		if (!fs.existsSync(item.from)) return;
		fs.watch(item.from, () => {
			clearTimeout(changeHandler);
			if (locked) return;
			changeHandler = setTimeout(() => {
				if (item.from === item.to) {
					locked = true;
					setTimeout(() => {
						locked = false;
					}, fsWatch_evHold);
				}
				printTime();
				bundle_css(item);
			}, fsWatch_evHold);
		});
	}
});


/*
	Report and call it a day
*/
if (flags.watch) console.log('\r\n', colorText(' Watching for file changes ', 'green', 'reverse'), '\r\n');