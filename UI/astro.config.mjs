import {defineConfig} from "astro/config";
import electron from "astro-electron";
import tailwind from "@astrojs/tailwind";

export default defineConfig({
  integrations: [electron(), tailwind()],
  build: {
    format: "file",
  }
});