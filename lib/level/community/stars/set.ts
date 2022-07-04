import get from './get'
import store from '.'

const setCommunityStars = (level: string, stars: number) => {
	const map = get()

	if (!level) throw new Error('Invalid level')

	map[level] = Math.max(map[level] ?? 0, stars)
	localStorage.setItem('communityStars', JSON.stringify(map))

	store.set(map)
}

export default setCommunityStars
