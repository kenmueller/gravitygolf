<script context="module" lang="ts">
	export const load: Load = ({ url }) => ({
		props: { url }
	})
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'
	import { getAnalytics, logEvent } from 'firebase/analytics'

	import { browser } from '$app/env'

	import app from '$lib/firebase'
	import overlay from '$lib/overlay/store'
	import MetaBase from '../components/Meta/Base.svelte'
	import PageTransition from '../components/Transition/Page.svelte'
	import Overlay from '../components/Overlay.svelte'

	export let url: URL

	$: if (browser)
		logEvent(getAnalytics(app), 'screen_view', {
			firebase_screen: url.pathname,
			firebase_screen_class: 'layout'
		})

	const down = (event: MouseEvent) => {
		if (
			event.target instanceof HTMLAnchorElement ||
			event.target instanceof HTMLButtonElement
		)
			event.preventDefault()
	}
</script>

<svelte:body on:mousedown={down} />

<svelte:head>
	<link
		rel="preconnect"
		href="https://fonts.googleapis.com"
		crossorigin="anonymous"
	/>
	<link
		rel="preconnect"
		href="https://fonts.gstatic.com"
		crossorigin="anonymous"
	/>
	<link
		rel="stylesheet"
		href="https://fonts.googleapis.com/css2?family=Kdam+Thmor+Pro&display=swap"
		crossorigin="anonymous"
	/>
	<!-- <script
		async
		src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client={encodeURIComponent(
			import.meta.env.VITE_ADSENSE_CLIENT
		)}"
		crossorigin="anonymous"></script> -->
</svelte:head>

<MetaBase />

<PageTransition {url} hidden={$overlay !== null}>
	<slot />
</PageTransition>

<Overlay />

<style lang="scss" global>
	@use 'balloon-css/src/balloon' as *;

	:root {
		--balloon-color: white;
		--balloon-text-color: colors.$gray;
	}

	[aria-label][data-balloon-pos]::after {
		font-family: 'Kdam Thmor Pro', -apple-system, BlinkMacSystemFont, 'Segoe UI',
			Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue',
			sans-serif !important;
	}

	*,
	::before,
	::after {
		box-sizing: border-box;
		margin: 0;
		padding: 0;
		font-family: 'Kdam Thmor Pro', -apple-system, BlinkMacSystemFont, 'Segoe UI',
			Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue',
			sans-serif;
		border: none;
		outline: none;
	}

	html,
	body {
		height: 100%;
	}

	body {
		user-select: none;
		touch-action: none;
		overflow: hidden;
		background: colors.$gray;
	}

	a {
		text-decoration: none;
	}

	button {
		cursor: pointer;
		background: none;

		&:disabled {
			cursor: default;
		}
	}

	svg {
		display: block;
	}
</style>
