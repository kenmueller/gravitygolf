import { cwd } from 'process'
import { join } from 'path'
import { writeFile } from 'fs/promises'
import fetch from 'node-fetch'

const data = join(cwd(), 'data')

const main = async () => {
	const response = await fetch('https://admin.grav.golf/api/levels')
	if (!response.ok) throw new Error(await response.text())

	const levels = await response.json()

	await Promise.all([
		writeFile(join(data, 'levels.json'), JSON.stringify(levels)),
		writeFile(join(data, 'level-count.json'), JSON.stringify(levels.length))
	])
}

void main()
