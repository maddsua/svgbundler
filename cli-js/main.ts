#!/usr/bin/env node

/*

    CopyLeft 2023 maddsua
    https://github.com/maddsua/svgbundler
    License - No license (specified MIT but not really)
    Use it however you want
    I don't guarantee anything, but at very least, this package is safe from dependency-related security issues

*/

import fs from 'fs';
import path from 'path';
import { globSync } from 'glob';
import chalk from 'chalk';


const minSvgLength = 32;
const fsWatch_evHold = 100;
const styleSourceFile = /\.s?css$/;
const rootPathRegex = /^[\\\/]+/;


/*
	Console stuff
*/
const printTime = () => {
	console.log(`[${new Date().toLocaleTimeString()}]`);
};


/*
	Flags and other stuff
*/
const flags = {
	optimize: false,
	watch: false,
	silent: false,
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

			//	path transformations
			item.from = item.from.replace(rootPathRegex, '');
			item.to = item.to.replace(rootPathRegex, '');
	
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
		(match[0].match(styleSourceFile) ? sources.css : sources.svg).push({
			from: path.normalize(match[0]).replace(rootPathRegex, ''),
			to: path.normalize(match[1]).replace(rootPathRegex, '')
		});
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
	console.error(chalk.red('Specify at least one input directory+destination file or a .css to bundle'), '\r\n');
	console.error(chalk.black.bgRed(' Build aborted '));
	process.exit(1);
}


/*
	Notify that we will add prefixes
*/
if (classPrefixText.length) {
	console.log('Prefix', chalk.yellow(classPrefixText), 'will be added to all the classes\r\n');
}


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
const bundle_svgdir = (svgDir: i_pathPair) => {
	console.log(chalk.green(`Compiling to ${svgDir.to} `));

	let writeContents = '';
	let imagesLoaded = 0;

	globSync(path.normalize(`${svgDir.from}/**/*.svg`).replace(/\\/g, '/')).forEach((svgFile) => {

		svgFile = path.normalize(svgFile);

		let svgtext = '';
		try { svgtext = fs.readFileSync(svgFile, {encoding: 'utf8'}) }
		catch (error) {
			console.warn(chalk.red(' Cant\'t read file:'), svgFile);
			return '';
		}

		let classname = (() => {

			let temp = svgFile.slice(svgDir.from.length);

			if (flags.flatten) temp = temp.replace(/-/g, '_');

			let classPrefix = svgDir.prefix ? svgDir.prefix : classPrefixText;
				if (classPrefix.length) classPrefix += '-';

			return `.${classPrefix}${temp.replace(/[\/\\]{1,}/g, '-')}`;
		})();

		if (flags.optimize) svgtext = minify(svgtext);

		if (svgtext.length < minSvgLength) {
			console.warn(chalk.yellow(` -! ${svgFile}`), '\r\n');
			return;
		}

		writeContents += svgDir.selector ? `${classname}:${svgDir.selector}` : `${classname}, ${classname}-after::after, ${classname}-before::before`;

		writeContents += ` {\r\n\tbackground-image: url(\"data:image/svg+xml,${encodeURIComponent(svgtext)}\")${svgDir.override ? '!important' : ''};\r\n}\r\n`;

		if (!flags.silent) console.log(` --> ${svgFile}`);
		imagesLoaded++;
	});

	if (writeContents.length < minSvgLength) {
		console.warn(chalk.yellow('No svgs bundled'), '\r\n');
		return;
	}

	const destDir = path.dirname(svgDir.to);
		if (!fs.existsSync(destDir)) fs.mkdirSync(destDir, { recursive: true });
	fs.writeFileSync(svgDir.to, writeContents, {encoding: 'utf8'});

	if (flags.silent) console.log(chalk.green(`+ ${imagesLoaded} images`), '\r\n');
		else  console.log(chalk.green('...done'), '\r\n');
};


/*
	Stuff to do the job a lil differently
*/
const bundle_cssfile = (file: i_pathPair) => {
	console.log(chalk.green(`Bundling ${file.from} to ${file.to} `));

	let cssText = '';
	try {
		cssText = fs.readFileSync(file.from, {encoding: 'utf-8'})
	} catch (error) { 
		console.warn(chalk.black.bgYellow(' Cannot read: '), chalk.yellow(file.from));
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

		const fsReadPath = path.normalize(regexes.ascend.test(providedPath) ? (path.dirname(file.from) + '\\' + providedPath) : providedPath).replace(rootPathRegex, '');

		let svgtext = '';

		try { 
			svgtext = fs.readFileSync(fsReadPath, {encoding: 'utf-8'});
		} catch (error) { 
			console.warn(chalk.yellow(` -! ${fsReadPath}`));
			return;
		}

		if (flags.optimize) svgtext = minify(svgtext);

		if (svgtext.length < minSvgLength) return;

		cssText = cssText.replace(image, `url("data:image/svg+xml,${encodeURIComponent(svgtext)}")`);
		
		if (!flags.silent) console.log(` --> ${fsReadPath}`);
		imagesLoaded++;
	});

	const destDir = path.dirname(file.to);
		if (!fs.existsSync(destDir)) fs.mkdirSync(destDir, { recursive: true });
	fs.writeFileSync(file.to, cssText, {encoding: 'utf8'});

	if (imagesLoaded) {
		if (flags.silent) console.log(chalk.green(`+ ${imagesLoaded} images`), '\r\n');
			else console.log(chalk.green('...done'), '\r\n');
		
	} else console.log(chalk.yellow('...nothing to do here'), '\r\n');
}


/*
	Add watchdogs for svg sources
*/
sources.svg.forEach((item) => {

	bundle_svgdir(item);

	if (!flags.watch) return;
	if (!fs.existsSync(item.from)) return;

	let changeHandler: NodeJS.Timeout | number = 0;

	fs.watch(item.from, () => {

		clearTimeout(changeHandler);
		changeHandler = setTimeout(() => {

			printTime();
			bundle_svgdir(item);

		}, fsWatch_evHold);
	});
	
});


/*
	Add watchdogs for css sources
*/
sources.css.forEach((item) => {

	bundle_cssfile(item);

	if (!flags.watch) return;
	if (!fs.existsSync(item.from)) return;

	let locked = false;
	let changeHandler: NodeJS.Timeout | number = 0;

	fs.watch(item.from, () => {

		clearTimeout(changeHandler);

		if (locked) return;

		changeHandler = setTimeout(() => {

			if (item.from === item.to) {

				locked = true;
				setTimeout(() => locked = false, fsWatch_evHold);
			}

			printTime();
			bundle_cssfile(item);

		}, fsWatch_evHold);
	});

});


/*
	Report and call it a day
*/
if (flags.watch) console.log('\r\n', chalk.black.bgGreen(' Watching for file changes '), '\r\n');