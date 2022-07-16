import { set } from '$lib/storage'
import getStars from './get'
import store from '.'

const setStars = (level: number, stars: number) => {
	try {
		const list = getStars()

		if (level - 1 < 0 || level - 1 > list.length)
			throw new Error('Invalid level')

		list[level - 1] = Math.max(list[level - 1] ?? 0, stars)
		set('stars', JSON.stringify(list))

		store.set(list)
	} catch (error) {
		console.error(error)
	}
}

export default setStars
