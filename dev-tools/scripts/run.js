#!/usr/bin/env -S node --input-type=module
/* eslint-disable camelcase, max-lines-per-function, jsdoc/require-jsdoc, jsdoc/require-param-description */
/*
This file is purposely large to easily move the code to multiple projects, its build code, not production.
To help navigate this file is divided by sections:
@section 1 init
@section 2 tasks
@section 3 jobs
@section 4 utils
@section 5 Dev Server
@section 6 linters
@section 7 minifiers
@section 8 exec utilities
@section 9 filesystem utilities
@section 10 npm utilities
@section 11 versioning utilities
@section 12 badge utilities
@section 13 module graph utilities
@section 14 build tools plugins
*/
import process from 'node:process'
import fs, { readFile as fsReadFile, writeFile } from 'node:fs/promises'
import { existsSync } from 'node:fs'
import { promisify } from 'node:util'
import { execFile as baseExecFile, exec as baseExec, spawn } from 'node:child_process'
const exec = promisify(baseExec)
const execFile = promisify(baseExecFile)
const readFile = (path) => fsReadFile(path, { encoding: 'utf8' })

// @section 1 init

const projectPathURL = new URL('../../', import.meta.url)
const pathFromProject = (path) => new URL(path, projectPathURL).pathname
process.chdir(pathFromProject('.'))

// @section 2 tasks

const helpTask = {
  description: 'show this help',
  cb: async () => { console.log(helpText()); process.exit(0) },
}

const tasks = {
  docs: {
    description: 'build documentation',
    cb: async () => { await buildDocs(); process.exit(0) },
  },
  help: helpTask,
  '--help': helpTask,
  '-h': helpTask,
}

async function main () {
  const args = process.argv.slice(2)
  if (args.length <= 0) {
    console.log(helpText())
    return process.exit(0)
  }

  const taskName = args[0]

  if (!Object.hasOwn(tasks, taskName)) {
    console.error(`unknown task ${taskName}\n\n${helpText()}`)
    return process.exit(1)
  }

  await checkNodeModulesFolder()
  await tasks[taskName].cb()
  return process.exit(0)
}

await main()

// @section 3 jobs


async function buildDocs () {
  logStartStage('build:docs', 'build docs')

  await rm_rf('build-docs')
  await cp_R('docs', 'build-docs')
  await cp_R('build-test/meson-logs', 'build-docs/reports')
  await cp_R('build-test/meson-logs', 'build-docs/reports')
  await cp_R('tests/ui', 'build-docs/reports/ui')
   await createBadges()
  await Promise.all([
    exec(`${process.argv[0]} dev-tools/scripts/build-html.js index.html`),
    exec(`${process.argv[0]} dev-tools/scripts/build-html.js contributing.html`),
  ])

  let uiTestContent = ""
  for (const dirPath of await listFolders('build-docs/reports/ui', {filterPatterns: ["UIT*"]})) {
    const folderName = dirPath.split("/").at(-1)
    console.log(folderName)
    await mv(`${dirPath}/script`, `${dirPath}/script.txt`)
    uiTestContent += await generateUITestBlock(`reports/ui/${folderName}`)
  }
  await writeFile(`build-docs/reports/ui/reports.html`, uiTestContent)
  await exec(`${process.argv[0]} dev-tools/scripts/build-html.js test-report.html`),

  logEndStage()
}

async function createBadges () {
  await makeBadgeForLicense(pathFromProject('build-docs/reports'))
  await makeBadgeForCoverages(pathFromProject('build-docs/reports'))
  await makeBadgeForTestResult(pathFromProject('build-docs/reports'))
  await makeBadgeForRepo(pathFromProject('build-docs/reports'))
  await makeBadgeForRelease(pathFromProject('build-docs/reports'))
}

// @section 4 utils

function helpText () {
  const fromNPM = isRunningFromNPMScript()

  const helpArgs = fromNPM ? 'help' : 'help, --help, -h'
  const maxTaskLength = Math.max(...[helpArgs, ...Object.keys(tasks)].map(text => text.length))
  const tasksToShow = Object.entries(tasks).filter(([_, value]) => value !== helpTask)
  const usageLine = fromNPM ? 'npm run <task>' : 'run <task>'
  return `Usage: ${usageLine}

Tasks: 
  ${tasksToShow.map(([key, value]) => `${key.padEnd(maxTaskLength, ' ')}  ${value.description}`).join('\n  ')}
  ${'help, --help, -h'.padEnd(maxTaskLength, ' ')}  ${helpTask.description}`
}

/** @param {string[]} paths  */
async function rm_rf (...paths) {
  await Promise.all(paths.map(path => fs.rm(path, { recursive: true, force: true })))
}

/** @param {string[]} paths  */
async function mkdir_p (...paths) {
  await Promise.all(paths.map(path => fs.mkdir(path, { recursive: true })))
}

/**
 * @param {string} src
   @param {string} dest  */
async function cp_R (src, dest) {
  await cmdSpawn(`cp -r '${src}' '${dest}'`)

  // this command is a 1000 times slower that running the command, for that reason it is not used (30 000ms vs 30ms)
  // await fs.cp(src, dest, { recursive: true })
}

async function mv (src, dest) {
  await fs.rename(src, dest)
}

function logStage (stage) {
  logEndStage(); logStartStage(logStage.currentJobName, stage)
}

function logEndStage () {
  const startTime = logStage.perfMarks[logStage.currentMark]
  console.log(startTime ? `done (${Date.now() - startTime}ms)` : 'done')
}

function logStartStage (jobname, stage) {
  const markName = 'stage ' + stage
  logStage.currentJobName = jobname
  logStage.currentMark = markName
  logStage.perfMarks ??= {}
  stage && process.stdout.write(`[${jobname}] ${stage}...`)
  logStage.perfMarks[logStage.currentMark] = Date.now()
}

// @section 5 Dev server

// @section 6 linters

// @section 7 minifiers

// @section 8 exec utilities

/**
 * @param {string} command
 * @param {import('node:child_process').ExecFileOptions} options
 * @returns {Promise<number>} code exit
 */
function cmdSpawn (command, options = {}) {
  const p = spawn('/bin/sh', ['-c', command], { stdio: 'inherit', ...options })
  return new Promise((resolve) => { p.on('exit', resolve) })
}

async function execCmd (command, args) {
  const options = {
    cwd: process.cwd(),
    env: process.env,
    stdio: 'pipe',
    encoding: 'utf-8',
  }
  return await execFile(command, args, options)
}

async function execGitCmd (args) {
  return (await execCmd('git', args)).stdout.trim().toString().split('\n')
}

// @section 9 filesystem utilities

async function listNonIgnoredFiles ({ ignorePath = '.gitignore', patterns } = {}) {
  const { minimatch } = await import('minimatch')
  const { join } = await import('node:path')
  const { statSync, readdirSync } = await import('node:fs')
  const ignorePatterns = await getIgnorePatternsFromFile(ignorePath)
  const ignoreMatchers = ignorePatterns.map(pattern => minimatch.filter(pattern, { matchBase: true }))
  const listFiles = (dir) => readdirSync(dir).reduce(function (list, file) {
    const name = join(dir, file)
    if (file === '.git' || ignoreMatchers.some(match => match(name))) { return list }
    const isDir = statSync(name).isDirectory()
    return list.concat(isDir ? listFiles(name) : [name])
  }, [])

  const fileList = listFiles('.')
  if (!patterns) { return fileList }
  const intersection = patterns.flatMap(pattern => minimatch.match(fileList, pattern, { matchBase: true, dot: true }))
  return [...new Set(intersection)]
}

async function getIgnorePatternsFromFile (filePath) {
  const content = await fs.readFile(filePath, 'utf8')
  const lines = content.split('\n').filter(line => !line.startsWith('#') && line.trim() !== '')
  return [...new Set(lines)]
}

async function listChangedFilesMatching (...patterns) {
  const { minimatch } = await import('minimatch')
  const changedFiles = [...(await listChangedFiles())]
  const intersection = patterns.flatMap(pattern => minimatch.match(changedFiles, pattern, { matchBase: true }))
  return [...new Set(intersection)]
}

async function listChangedFiles () {
  const mainBranchName = 'main'
  const mergeBase = await execGitCmd(['merge-base', 'HEAD', mainBranchName])
  const diffExec = execGitCmd(['diff', '--name-only', '--diff-filter=ACMRTUB', mergeBase])
  const lsFilesExec = execGitCmd(['ls-files', '--others', '--exclude-standard'])
  return new Set([...(await diffExec), ...(await lsFilesExec)].filter(filename => filename.trim().length > 0))
}

async function listFolders(dir, {filterPatterns = []} = {}){
  const { minimatch } = await import('minimatch')
  const { join } = await import('node:path')
  const { statSync, readdirSync } = await import('node:fs')
  const dirList = readdirSync(dir).reduce(function (list, file) {
    const name = join(dir, file)
    const isDir = statSync(name).isDirectory()
    if(!isDir) { return list }
    return list.concat(name)
  }, [])

  if (!filterPatterns) { return fileList }
  const intersection = filterPatterns.flatMap(pattern => minimatch.match(dirList, pattern, { matchBase: true, dot: true }))
  return [...new Set(intersection)]

}

function isRunningFromNPMScript () {
  return false
}

// @section 10 npm utilities

async function checkNodeModulesFolder () {
  if (existsSync(pathFromProject('dev-tools/scripts/node_modules'))) { return }
  console.log('node_modules absent running "npm ci"...')
  await cmdSpawn('npm ci', {cwd: pathFromProject('dev-tools/scripts')})
}


// @section 11 versioning utilities

async function getLatestReleasedVersion () {
  const changelogContent = await readFile("CHANGELOG")
  const versions = changelogContent.split('\n')
    .map(line => {
      const match = line.match(/^## \[([0-9]+\.[[0-9]+\.[[0-9]+)]\s+-\s+([^\s]+)/)
      if(!match){
        return null
      }
      return {version: match[1], releaseDate: match[2]}
    }).filter(version => !!version)
  const releasedVersions = versions.filter(version => {
    return version.releaseDate.match(/[0-9]{4}-[0-9]{2}-[0-9]{2}/)
  })
  return releasedVersions[0]
}

// @section 12 badge utilities

function getBadgeColors () {
  getBadgeColors.cache ??= {
    green: '#007700',
    yellow: '#777700',
    orange: '#aa0000',
    red: '#aa0000',
    blue: '#007ec6',
  }
  return getBadgeColors.cache
}

function asciiIconSvg (asciicode) {
  return `data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 10 10'%3E%3Cstyle%3Etext %7Bfont-size: 10px; fill: %23333;%7D @media (prefers-color-scheme: dark) %7Btext %7B fill: %23ccc; %7D%7D %3C/style%3E%3Ctext x='0' y='10'%3E${asciicode}%3C/text%3E%3C/svg%3E`
}

async function makeBadge (params) {
  const { default: libMakeBadge } = await import('badge-maker/lib/make-badge.js')
  return libMakeBadge({
    style: 'for-the-badge',
    ...params,
  })
}

function getLightVersionOfBadgeColor (color) {
  const colors = getBadgeColors()
  getLightVersionOfBadgeColor.cache ??= {
    [colors.green]: '#90e59a',
    [colors.yellow]: '#dd4',
    [colors.orange]: '#fa7',
    [colors.red]: '#f77',
    [colors.blue]: '#acf',
  }
  return getLightVersionOfBadgeColor.cache[color]
}

function badgeColor (pct) {
  const colors = getBadgeColors()
  if (pct > 80) { return colors.green }
  if (pct > 60) { return colors.yellow }
  if (pct > 40) { return colors.orange }
  if (pct > 20) { return colors.red }
  return 'red'
}

async function svgStyle () {
  const { document } = await loadDom()
  const style = document.createElement('style')
  style.innerHTML = `
  text { fill: #333; }
  .icon {fill: #444; }
  rect.label { fill: #ccc; }
  rect.body { fill: var(--light-fill); }
  @media (prefers-color-scheme: dark) {
    text { fill: #fff; }
    .icon {fill: #ccc; }
    rect.label { fill: #555; stroke: none; }
    rect.body { fill: var(--dark-fill); }
  }
  `.replaceAll(/\n+\s*/g, '')
  return style
}

async function applyA11yTheme (svgContent, options = {}) {
  const { document } = await loadDom()
  const { body } = document
  body.innerHTML = svgContent
  const svg = body.querySelector('svg')
  if (!svg) { return svgContent }
  svg.querySelectorAll('text').forEach(el => el.removeAttribute('fill'))
  if (options.replaceIconToText) {
    const img = svg.querySelector('image')
    if (img) {
      const text = document.createElementNS('http://www.w3.org/2000/svg', 'text')
      text.textContent = options.replaceIconToText
      text.setAttribute('transform', 'scale(.15)')
      text.classList.add('icon')
      text.setAttribute('x', '90')
      text.setAttribute('y', '125')
      img.replaceWith(text)
    }
  }
  const rects = Array.from(svg.querySelectorAll('rect'))
  rects.slice(0, 1).forEach(el => {
    el.classList.add('label')
    el.removeAttribute('fill')
  })
  const colors = getBadgeColors()
  let color = colors.red
  rects.slice(1).forEach(el => {
    color = el.getAttribute('fill') || colors.red
    el.removeAttribute('fill')
    el.classList.add('body')
    el.style.setProperty('--dark-fill', color)
    el.style.setProperty('--light-fill', getLightVersionOfBadgeColor(color))
  })
  svg.prepend(await svgStyle())

  return svg.outerHTML
}

async function makeBadgeForCoverages (path) {
  const coverateReport = await readFile(`${path}/coverage.txt`)
  const percentage = coverateReport.split('\n')
    .filter(line => line.startsWith("TOTAL"))
    .map(line => line.replace(/TOTAL\s+[0-9]+\s+[0-9]+\s+([0-9]+)%.*/, "$1") )
    [0]
  const svg = await makeBadge({
    label: 'coverage',
    message: `${percentage}%`,
    color: badgeColor(percentage),
    logo: asciiIconSvg('🛡︎'),
  })

  const badgeWrite = writeFile(`${path}/coverage-badge.svg`, svg)
  const a11yBadgeWrite = writeFile(`${path}/coverage-badge-a11y.svg`, await applyA11yTheme(svg, { replaceIconToText: '🛡︎' }))
  await Promise.all([badgeWrite, a11yBadgeWrite])
}


async function makeBadgeForRepo(path){
    const svg = await makeBadge({
      label: 'Code Repository',
      message: 'Github',
      color: getBadgeColors().blue,
      logo: asciiIconSvg('❮❯'),
    })
  const badgeWrite = writeFile(`${path}/repo-badge.svg`, svg)
  const a11yBadgeWrite = writeFile(`${path}/repo-badge-a11y.svg`, await applyA11yTheme(svg, { replaceIconToText: '❮❯' }))
  await Promise.all([badgeWrite, a11yBadgeWrite])
}

async function makeBadgeForRelease(path){
  const releaseVersion = await getLatestReleasedVersion()
  const svg = await makeBadge({
    label: 'Release',
    message: releaseVersion ? releaseVersion.version : "Unreleased",
    color: getBadgeColors().blue,
    logo: asciiIconSvg('⛴'),
  })
const badgeWrite = writeFile(`${path}/repo-release.svg`, svg)
const a11yBadgeWrite = writeFile(`${path}/repo-release-a11y.svg`, await applyA11yTheme(svg, { replaceIconToText: '⛴' }))
await Promise.all([badgeWrite, a11yBadgeWrite])
}

makeBadgeForRelease

async function makeBadgeForTestResult (path) {
  const stdout = await readFile(`${path}/testlog.json`).then(str => str.split("\n").map(line => line ? JSON.parse(line).stdout: "").join(''))
  const tests = stdout.split('\n').filter(test => /^n?ok /.test(test) )
  const passedTests = tests.filter(test => test.startsWith('ok'))
  const testAmountFromTap = stdout.split('\n')
    .filter(test => /^1../.test(test) )
    .map(line => +line.split('..')[1] ?? 0)
    .reduce((a, b) => a + b)
  const testAmount =  testAmountFromTap || tests.length
  const passedAmount = passedTests.length
  const passed = passedAmount === testAmount
  const svg = await makeBadge({
    label: 'tests',
    message: `${passedAmount} / ${testAmount}`,
    color: passed ? '#007700' : '#aa0000',
    logo: asciiIconSvg('✔'),
    logoWidth: 16,
  })
  const badgeWrite = writeFile(`${path}/test-results-badge.svg`, svg)
  const a11yBadgeWrite = writeFile(`${path}/test-results-badge-a11y.svg`, await applyA11yTheme(svg, { replaceIconToText: '✔' }))
  await Promise.all([badgeWrite, a11yBadgeWrite])
}

async function makeBadgeForLicense (path) {
  const svg = await makeBadge({
    label: ' license',
    message: 'LGPL',
    color: '#007700',
    logo: asciiIconSvg('🏛'),
  })

  const badgeWrite = writeFile(`${path}/license-badge.svg`, svg)
  const a11yBadgeWrite = writeFile(`${path}/license-badge-a11y.svg`, await applyA11yTheme(svg, { replaceIconToText: '🏛' }))
  await Promise.all([badgeWrite, a11yBadgeWrite])
}

async function loadDom () {
  if (!loadDom.cache) {
    loadDom.cache = import('jsdom').then(({ JSDOM }) => {
      const jsdom = new JSDOM('<body></body>', { url: import.meta.url })
      const window = jsdom.window
      const DOMParser = window.DOMParser
      /** @type {Document} */
      const document = window.document
      return { window, DOMParser, document }
    })
  }
  return loadDom.cache
}

// @section 13 module graph utilities

// @section 14 build tools plugins

async function generateUITestBlock(foldername){
  const escapeHtml = (s) => s.replace(/[^0-9A-Za-z ]/g,c => "&#" + c.charCodeAt(0) + ";");
  const testDescription = await readFile(`build-docs/${foldername}/DESCRIPTION`)
  const [testName, ...description] = testDescription.split('\n').map(escapeHtml)


  return `
  <h3>${testName}</h3>
  <div>${description.join('\n')}</div>
<section>
    <div>
        <script type="text/plain" class="bash-example" ss:include="${foldername}/script.txt"></script>
    </div>
    <div class="caption">Rofi block script</div>
</section>

<section>
    <!-- Comparison Slider - this div contain the slider with the individual images captions -->
    <div class="comparison-slider">
        <div class="overlay"><strong>Expected</strong> image.</div>
    <img src="${foldername}/expected-screenshot-1.png" alt="expected screenshot">
    <!-- Div containing the image layed out on top from the left -->
    <div class="resize">
        <div class="overlay"> <strong>Result</strong> image.</div>
        <img src="${foldername}/result-screenshot-1.png" alt="result screenshot">
    </div>
    <!-- Divider where user will interact with the slider -->
    <div class="divider"></div>
    </div>
    <!-- All global captions if exist can go on the div bellow -->
    <div class="caption">image slider</div>
</section>

<section>
    <img class="diff-image" src="${foldername}/diff-screenshot-1.png" alt="diffence image">
    <div class="caption">Difference image</div>
</section>
`

} 