!function(){"use strict";var a=window.document,b={STYLES:"https://c.disquscdn.com/next/recommendations/styles/recommendations.10022a97346f1c6e3798931bbd8e4bb5.css",RTL_STYLES:"https://c.disquscdn.com/next/recommendations/styles/recommendations_rtl.9c3ccad48c24340222b5a85c20ea36b8.css","recommendations/main":"https://c.disquscdn.com/next/recommendations/recommendations.bundle.bb3216316047d5c61d9dafa6240fbf39.js","remote/config":"https://disqus.com/next/config.js"};window.require={baseUrl:"https://c.disquscdn.com/next/current/recommendations/recommendations",paths:["recommendations/main","remote/config"].reduce(function(a,c){var d=-3;return a[c]=b[c].slice(0,d),a},{})};var c=a.createElement("script");c.onload=function(){require(["common/main"],function(a){a.init("recommendations",b)})},c.src="https://c.disquscdn.com/next/recommendations/common.bundle.3135ee5a6409396e26a2daa5f1d7a9fc.js",a.body.appendChild(c)}();