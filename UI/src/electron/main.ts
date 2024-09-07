import {app, BrowserWindow, ipcMain} from "electron";
import path from "path";

const cn = require("bindings")("chatnotifier");

const chatNotifierMethods = {
  printer: cn.printer,
  initialize: cn.initialize,
  cleanup: cn.cleanup,
  save_config: cn.save_config,
  get_config_json: cn.get_config_json,
  set_config_json: cn.set_config_json,
  connect_twitch: cn.connect_twitch,
  disconnect_twitch: cn.disconnect_twitch,
  initialized: cn.initialized,
  get_twitch_connection_status: cn.get_twitch_connection_status,
  stop_all_sounds: cn.stop_all_sounds,
  find_new_assets: cn.find_new_assets,
  reload_scripts: cn.reload_scripts,
};

function createWindow() {
  const win = new BrowserWindow({
    width: 690,
    height: 420,
    webPreferences: {
      nodeIntegration: true,
      nodeIntegrationInWorker: true,
      preload: path.join(__dirname, "preload.js"),
    },
    autoHideMenuBar: true,
  });

  win.loadFile("dist/index.html").catch(console.error);
}

app.whenReady().then(() => {
  if (!cn.initialized()) {
    cn.printer("ChatNotifier initializing with cwd.. " + process.cwd());
    cn.initialize(process.cwd());
    cn.printer("ChatNotifier initialized");
  }

  createWindow()
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0)
      createWindow();
  });

  ipcMain.handle("chatnotifier-call", (event, args) => {
    const methodName = args.method;
    const params = args.params;

    if (chatNotifierMethods[methodName] === undefined)
      return Promise.reject(new Error(`Method ${methodName} not found`));

    if (params === undefined) {
      cn.printer(`Calling ${methodName} with no params`);
      return chatNotifierMethods[methodName]();
    }
    else if (Array.isArray(params)) {
      cn.printer(`Calling ${methodName} with ...params: ${params}`);
      return chatNotifierMethods[methodName](...params);
    }
    else {
      cn.printer(`Calling ${methodName} with params: ${params}`);
      return chatNotifierMethods[methodName](params);
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin")
    app.quit();
});
