import createClient from 'algoliasearch'

const search = createClient(
	import.meta.env.VITE_ALGOLIA_APP_ID,
	import.meta.env.VITE_ALGOLIA_API_KEY
)

export default search
