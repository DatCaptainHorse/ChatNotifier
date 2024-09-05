import {contextBridge} from 'electron';

const cn = require("bindings")("chatnotifier");

if (!cn.initialized()) {
  cn.printer("ChatNotifier initializing with cwd.. " + process.cwd());
  cn.initialize(process.cwd());
  cn.printer("ChatNotifier initialized");
}

contextBridge.exposeInMainWorld("chatnotifier", {
  instance: cn,
});
