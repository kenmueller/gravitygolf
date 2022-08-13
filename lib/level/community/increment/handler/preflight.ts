import type { RequestHandler } from '@sveltejs/kit'

const incrementCommunityLevelPropertyPreflightHandler: RequestHandler = () => ({
	headers: {
		'access-control-allow-methods': 'OPTIONS, POST',
		'access-control-allow-origin': '*',
		'access-control-allow-credentials': 'true'
	},
	body: ''
})

export default incrementCommunityLevelPropertyPreflightHandler
