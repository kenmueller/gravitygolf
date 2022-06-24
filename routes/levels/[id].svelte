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
	import { onDestroy } from 'svelte'

	import type Level from '$lib/level'
	import Scene from '$lib/scene'
	import levels from '$lib/level/levels.json'
	import draggable from '$lib/draggable'
	import BackLink from '../../components/Link/Back.svelte'
	import Reset from '../../images/Reset.svelte'
	import Trash from '../../images/Trash.svelte'

	export let id: number
	$: level = levels[id - 1] as Level

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas?.getContext('2d')

	$: scene = canvas && context && new Scene(canvas, context, { id, ...level })

	onDestroy(() => {
		scene?.destroy()
	})
</script>

<svelte:head>
	<title>Level {id} | Gravity Golf</title>
</svelte:head>

<main>
	<header>
		<BackLink href="/levels">
			Level {id} | Gravity Golf
		</BackLink>
		<div>
			<span use:draggable={console.log} data-remaining="2" />
			<span use:draggable={console.log} data-remaining="2" />
		</div>
		<button class="reset" disabled={!scene} on:click={() => scene?.reset()}>
			<span>Press SPACE BAR to restart</span>
			<Reset />
		</button>
		<button class="clear" disabled={!scene} on:click={() => scene?.clear()}>
			<Trash />
		</button>
	</header>
	<canvas bind:this={canvas} />
</main>

<style lang="scss">
	main {
		position: relative;
		height: 100%;
		user-select: none;
	}

	header {
		display: flex;
		align-items: center;
		position: absolute;
		top: 1rem;
		left: 1rem;
		right: 1rem;
		z-index: 100;
		pointer-events: none;

		> :global(*) {
			pointer-events: all;
		}
	}

	[data-remaining] {
		$size: 40px;

		cursor: move;
		display: inline-block;
		width: $size;
		height: $size;
		background: blue;
		border-radius: 50%;
	}

	button {
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			width: 1.5rem;
		}
	}

	.reset {
		display: flex;
		align-items: center;
		margin-left: auto;

		> :global(svg) {
			margin-left: 0.7rem;
		}
	}

	.clear {
		margin-left: 1rem;
	}
</style>
