import type { User } from 'firebase/auth'

import errorFromResponse from '$lib/error/from/response'

const sendToken = async (user: User | null) => {
	const response = await fetch('/api/token', {
		method: 'POST',
		headers: { 'content-type': 'application/json' },
		body: JSON.stringify(user && (await user.getIdToken(true)))
	})

	if (!response.ok) throw await errorFromResponse(response)
}

export default sendToken
