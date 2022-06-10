<script context="module" lang="ts">
	export const load: Load = ({ params }) => {
		const id = Number.parseInt(params.id)

		if (Number.isNaN(id)) return { status: 301, redirect: '/levels' }

		if (id - 1 < 0) return { status: 301, redirect: '/levels' }
		if (id - 1 >= levels.length) return { status: 307, redirect: '/levels' }

		return {
			props: { id }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import { browser } from '$app/env'

	import levels from '$lib/level/levels.json'
	import BackLink from '../../components/Link/Back.svelte'

	export let id: number

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas && canvas.getContext('2d')

	let size: [number, number] | null = browser
		? [window.innerWidth, window.innerHeight]
		: null

	$: scale = browser ? window.devicePixelRatio : null
	$: scale && context?.scale(scale, scale)

	const onResize = () => {
		size = [window.innerWidth, window.innerHeight]
	}
</script>

<svelte:window on:resize={onResize} />

<svelte:head>
	<title>Level {id}</title>
</svelte:head>

<main>
	<canvas
		bind:this={canvas}
		width={size && scale && Math.floor(size[0] * scale)}
		height={size && scale && Math.floor(size[1] * scale)}
		style={size && `width: ${size[0]}px; height: ${size[1]}px;`}
	/>
	<BackLink href="/levels" />
</main>

<style lang="scss">
	main {
		position: relative;

		> :global(a) {
			position: absolute;
			top: 1rem;
			left: 1rem;
		}
	}
</style>
