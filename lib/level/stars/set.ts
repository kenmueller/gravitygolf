import get from './get'
import store from '.'

const setStars = (level: number, stars: number) => {
	const list = get()

	if (level - 1 < 0 || level - 1 > list.length) throw new Error('Invalid level')

	list[level - 1] = Math.max(list[level - 1] ?? 0, stars)
	localStorage.setItem('stars', JSON.stringify(list))

	store.set(list)
}

export default setStars
