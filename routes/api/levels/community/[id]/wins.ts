import incrementPreflightHandler from '$lib/level/community/increment/handler/preflight'
import incrementHandler from '$lib/level/community/increment/handler'

export const OPTIONS = incrementPreflightHandler
export const POST = incrementHandler('wins')
