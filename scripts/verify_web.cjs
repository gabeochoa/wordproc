const fs = require('node:fs');
const path = require('node:path');
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright');

(async () => {
  const output = path.resolve(process.argv[3] || 'output/upgrade/web');
  fs.mkdirSync(output, { recursive: true });
  const browser = await chromium.launch({
    channel: 'chrome', headless: true,
    args: ['--enable-unsafe-swiftshader', '--use-angle=swiftshader'],
  });
  const page = await browser.newPage({ viewport: { width: 1000, height: 700 } });
  const errors = [];
  const logs = [];
  page.on('pageerror', error => errors.push(error.stack));
  page.on('console', message => logs.push(`${message.type()}: ${message.text()}`));
  let failure;
  try {
    await page.goto(process.argv[2] || 'http://localhost:8795/wordproc.html');
    await page.locator('#loading').waitFor({ state: 'hidden', timeout: 60000 });
    await page.waitForTimeout(2000);
    await page.screenshot({ path: path.join(output, '01-ready.png') });
    await page.locator('canvas').click({ position: { x: 100, y: 220 } });
    await page.keyboard.type('Afterhours web verification');
    await page.waitForTimeout(400);
    await page.screenshot({ path: path.join(output, '02-typed.png') });
    await page.keyboard.press('Control+f', { delay: 100 });
    await page.waitForTimeout(400);
    await page.mouse.click(420, 302, { delay: 100 });
    await page.keyboard.type('web');
    await page.waitForTimeout(200);
    await page.screenshot({ path: path.join(output, '03-find.png') });
    await page.keyboard.press('Escape', { delay: 100 });
    await page.waitForTimeout(400);
    await page.setViewportSize({ width: 800, height: 600 });
    await page.waitForTimeout(500);
    await page.screenshot({ path: path.join(output, '04-resized.png') });
    if (errors.length) throw new Error(`${errors.length} browser runtime errors`);
  } catch (error) {
    failure = error.stack;
  } finally {
    const canvas = await page.locator('canvas').boundingBox().catch(() => null);
    const result = { success: !failure, failure, errors, canvas };
    fs.writeFileSync(path.join(output, 'verification.json'), JSON.stringify(result, null, 2));
    fs.writeFileSync(path.join(output, 'browser.log'), logs.join('\n'));
    console.log(JSON.stringify(result, null, 2));
    await browser.close();
    if (failure) process.exitCode = 1;
  }
})();
