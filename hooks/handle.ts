import type { Handle } from '@sveltejs/kit'

import type Locals from '$lib/locals'
import getToken from '$lib/token/get'
import userFromToken from '$lib/user/from/token'

const handle: Handle = async ({ event, resolve }) => {
	if (!MOBILE) {
		const cookies = event.request.headers.get('cookie') ?? ''
		const token = getToken(cookies)

		;(event.locals as Locals).user = token ? await userFromToken(token) : null
	}

	return resolve(event)
}

export default handle
