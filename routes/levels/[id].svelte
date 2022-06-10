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
	import Reset from '../../images/Reset.svelte'

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
	<title>Level {id} | Gravity Golf</title>
</svelte:head>

<main>
	<header>
		<BackLink href="/levels">
			Level {id} | Gravity Golf
		</BackLink>
		<button>
			<Reset />
		</button>
	</header>
	<canvas
		bind:this={canvas}
		width={size && scale && Math.floor(size[0] * scale)}
		height={size && scale && Math.floor(size[1] * scale)}
		style={size && `width: ${size[0]}px; height: ${size[1]}px;`}
	/>
</main>

<style lang="scss">
	main {
		position: relative;
		height: 100%;
	}

	header {
		display: flex;
		align-items: center;
		position: absolute;
		top: 1rem;
		left: 1rem;
		right: 1rem;
		z-index: 100;
	}

	button {
		margin-left: auto;
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			width: 1.5rem;
		}
	}
</style>
