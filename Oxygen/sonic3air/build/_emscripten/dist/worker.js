var _cacheName = 'sonic3air-v20210404';
var _cacheFiles = [
	'sonic3air_web.html',
	'sonic3air_web.js',
	'sonic3air_web.wasm',
	'loader.js',
	'manifest.json',
	'icon.png',
	'browserfs.min.js',
	'filemanager.js',
	'fileManagerRuntime.js',
	'react.js'
];
	
self.addEventListener('install', (e) => {
	console.log('[Service Worker] Install');
	e.waitUntil(
	    caches.open(_cacheName).then((cache) => {
	      console.log('[Service Worker] Caching all: Game executable and web files');
	      return cache.addAll(_cacheFiles);
	    })
	);
});

self.addEventListener('fetch', (e) => {
  e.respondWith(
    caches.match(e.request).then((r) => {
          console.log('[Service Worker] Fetching resource: '+e.request.url);
          return r || fetch(e.request).then((response) => {
                return caches.open(_cacheName).then((cache) => {
                	console.log('[Service Worker] Caching new resource: '+e.request.url);
                	cache.put(e.request, response.clone());
          			return response;
        });
      });
    })
  );
});