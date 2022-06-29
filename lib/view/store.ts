import type { Readable } from 'svelte/store'
import { derived } from 'svelte/store'

import { browser } from '$app/env'

import type View from '.'
import mobile from '$lib/mobile'
import landscape from '$lib/landscape'
import getView from './get'

const view = derived<
	[Readable<boolean>, Readable<boolean | null>],
	View | null
>([mobile, landscape], ([$mobile, $landscape], set) => {
	if (!browser) return set(null)

	const setView = () => {
		set({
			...getView(),
			mobile: $mobile,
			landscape: $landscape
		})
	}

	setView()
	window.addEventListener('resize', setView)

	return () => {
		window.removeEventListener('resize', setView)
	}
})

export default view
