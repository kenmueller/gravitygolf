import { cwd } from 'process'
import { join } from 'path'
import { mkdir, writeFile } from 'fs/promises'
import fetch from 'node-fetch'

const origin = process.env.ADMIN_ORIGIN || 'https://admin.grav.golf'
const data = join(cwd(), 'data')

void (async () => {
	const response = await fetch(new URL('/api/levels', origin))
	if (!response.ok) throw new Error(await response.text())

	const levels = await response.json()

	await mkdir(data, { recursive: true })

	await Promise.all([
		writeFile(join(data, 'levels.json'), JSON.stringify(levels)),
		writeFile(join(data, 'level-count.json'), JSON.stringify(levels.length))
	])
})()
