/// <reference lib="webworker" />

import {
	version,
	prerendered as staticPages,
	build as buildFiles,
	files as staticFiles
} from '$service-worker'

import levels from './level/levels'

type MaybePromise<Value> = Value | Promise<Value>

const CACHE = `cache/${version}`

const files = [
	...staticPages,
	...buildFiles,
	...staticFiles,
	'/',
	'/levels',
	...levels.map((_level, index) => `/levels/${index + 1}`),
	'/levels/editor'
]

const worker = self as unknown as ServiceWorkerGlobalScope

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
	const { request } = event
	if (request.method !== 'GET') return

	const { origin, pathname } = new URL(request.url)
	if (origin !== worker.location.origin) return

	event.respondWith(files.includes(pathname) ? stale(request) : fresh(request))
})

const stale = async (request: Request) =>
	(await fromCache(request)) ?? save(request, caches.open(CACHE))

const fresh = async (request: Request) => {
	try {
		return await save(request, caches.open(CACHE))
	} catch (error) {
		const cached = await fromCache(request)
		if (cached) return cached

		throw error
	}
}

const fromCache = async (request: Request) =>
	(await caches.match(request)) ?? null

const save = async (request: Request, cache: MaybePromise<Cache>) => {
	const response = await fetch(request)
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
