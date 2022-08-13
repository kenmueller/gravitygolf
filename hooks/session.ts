import type { GetSession } from '@sveltejs/kit'
import isMobile from 'is-mobile'

import type Session from '$lib/session'
import type Locals from '$lib/locals'

const getSession: GetSession = ({ request, locals }): Session => {
	const userAgent = request.headers.get('user-agent')

	return {
		user: (locals as Locals).user,
		mobile: MOBILE || (userAgent ? isMobile({ ua: userAgent }) : false)
	}
}

export default getSession
