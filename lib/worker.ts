/// <reference lib="webworker" />

import { version, files as _files } from '$service-worker'
import levels from './level/levels'

type MaybePromise<Value> = Value | Promise<Value>

const CACHE = `cache/${version}`

const worker = self as unknown as ServiceWorkerGlobalScope

const files = [
	..._files,
	'/',
	'/levels',
	...levels.map((_level, index) => `/levels/${index + 1}`),
	'/levels/editor'
]

worker.addEventListener('install', event => {
	event.waitUntil(
		caches.open(CACHE).then(async cache => {
			await cache.addAll(files)
			await worker.skipWaiting()
		})
	)
})

worker.addEventListener('activate', event => {
	event.waitUntil(
		caches.keys().then(async keys => {
			await Promise.all(keys.map(key => key === CACHE || caches.delete(key)))
			await worker.clients.claim()
		})
	)
})

worker.addEventListener('fetch', event => {
	if (event.request.method !== 'GET') return

	const url = new URL(event.request.url)

	if (url.origin !== worker.location.origin) return
	if (!files.includes(url.pathname)) return

	event.respondWith(stale(event.request))
})

const stale = async (request: Request) =>
	(await fromCache(request)) ?? save(request, caches.open(CACHE))

const fromCache = async (request: Request) => {
	const response = await caches.match(request)

	if (!response) return null
	if (!response.ok) throw new Error(await response.clone().text())

	return response
}

const save = async (request: Request, cache: MaybePromise<Cache>) => {
	const response = await fetch(request)
	if (!response.ok) throw new Error(await response.text())

	void saveTransaction(request, response.clone(), cache)

	return response
}

const saveTransaction = async (
	request: Request,
	response: Response,
	cache: MaybePromise<Cache>
) => {
	await (await cache).put(request, response)
}
