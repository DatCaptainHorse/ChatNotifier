import {contextBridge, ipcRenderer} from "electron";

contextBridge.exposeInMainWorld("chatnotifier", {
  call: async (method: string, params: any) => {
    return ipcRenderer.invoke('chatnotifier-call', {method, params});
  },
});
