import type Property from './property'
import HttpError from '$lib/error/http'
import ErrorCode from '$lib/error/code'

const incrementCommunityLevelProperty = (id: string, property: Property) => {
	const sent = navigator.sendBeacon(
		`/api/levels/community/${encodeURIComponent(id)}/${encodeURIComponent(
			property
		)}`
	)

	if (!sent)
		throw new HttpError(ErrorCode.Internal, `Unable to increment ${property}`)
}

export default incrementCommunityLevelProperty
