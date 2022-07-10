import type Property from './property'
import HttpError from '$lib/error'
import ErrorCode from '$lib/error/code'

const incrementCommunityLevelProperty = (id: string, property: Property) => {
	const sent = navigator.sendBeacon(
		`/communityLevels/${encodeURIComponent(id)}/${encodeURIComponent(property)}`
	)

	if (!sent)
		throw new HttpError(ErrorCode.Internal, `Unable to increment ${property}`)
}

export default incrementCommunityLevelProperty
