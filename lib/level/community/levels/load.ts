import type Ref from '$lib/ref'
import type Record from '../record'
import debounce from '$lib/debounce'
import get from './get'
import initial from './initial'

const loadCommunityLevels = debounce(
	async (
		query: string,
		set: (levels: Record[]) => void,
		cancel: Ref<boolean>
	) => {
		try {
			const levels = await get(fetch, query)
			if (cancel.current) return

			initial.set(levels)
			set(levels)
		} catch (error) {
			console.error(error)
		}
	},
	500
)

export default loadCommunityLevels
