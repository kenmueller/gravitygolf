import get from './get'
import store from '.'

const setCommunityStars = (level: string, stars: number) => {
	try {
		const map = get()

		if (!level) throw new Error('Invalid level')

		map[level] = Math.max(map[level] ?? 0, stars)
		localStorage.setItem('communityStars', JSON.stringify(map))

		store.set(map)
	} catch (error) {
		console.error(error)
	}
}

export default setCommunityStars
