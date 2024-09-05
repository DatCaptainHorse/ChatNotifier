import {app, BrowserWindow} from "electron";
import path from "path";

function createWindow() {
  const win = new BrowserWindow({
    width: 690,
    height: 420,
    webPreferences: {
      nodeIntegration: true,
      nodeIntegrationInWorker: true,
      preload: path.join(__dirname, "preload.js")
    },
    autoHideMenuBar: true,
  });

  win.loadFile('dist/index.html').catch(console.error);
}

app.whenReady().then(() => {
  createWindow()
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0)
      createWindow()
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin')
    app.quit();
});
