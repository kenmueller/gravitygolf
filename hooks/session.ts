import type { GetSession } from '@sveltejs/kit'
import isMobile from 'is-mobile'

import type Session from '$lib/session'

const getSession: GetSession = ({ request }): Session => {
	const userAgent = request.headers.get('user-agent')

	return {
		mobile: userAgent ? isMobile({ ua: userAgent }) : false
	}
}

export default getSession
