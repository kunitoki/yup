/** A script a page is given as it is written, which `esbuild.js` bundles as its text (see previewPage.ts). */
declare module "*.page.js" {
    const text: string;
    export default text;
}
