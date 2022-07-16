import { remove } from '$lib/storage'

const deleteLevelProgress = () => {
	try {
		remove('stars')
		remove('communityStars')
	} catch (error) {
		console.error(error)
	}
}

export default deleteLevelProgress
