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

	import { browser } from '$app/env'

	import type Position from '$lib/position'
	import FORCE_RADIUS from '$lib/scene/force/radius'
	import MAX_STARS from '$lib/scene/star/max'
	import Scene from '$lib/scene'
	import type RawLevel from '$lib/level/raw'
	import levelFromRaw from '$lib/level/raw/from'
	import levels from '$lib/level/levels.json'
	import draggable from '$lib/draggable'
	import BackLink from '../../components/Link/Back.svelte'
	import Reset from '../../images/Reset.svelte'
	import Trash from '../../images/Trash.svelte'

	import starImage from '../../images/star.png'

	export let id: number
	$: level = levelFromRaw(levels[id - 1] as RawLevel)

	$: forcesRemaining = {
		gravity: level.maxGravity - level.defaultGravity.length,
		antigravity: level.maxAntigravity - level.defaultAntigravity.length
	}

	$: stars = MAX_STARS - level.stars.length

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas?.getContext('2d')

	$: scene = canvas && context && new Scene(canvas, context, { id, ...level })

	$: scene?.addEventListener('forces', (gravity, antigravity) => {
		forcesRemaining = {
			gravity: level.maxGravity - gravity,
			antigravity: level.maxAntigravity - antigravity
		}
	})

	$: scene?.addEventListener('stars', newStars => {
		stars = newStars
	})

	const addForce = (direction: 1 | -1) => (position: Position) => {
		scene?.addForce(position, direction)
	}

	$: scale = browser ? window.devicePixelRatio : null

	const setScale = () => {
		scale = window.devicePixelRatio
	}

	onDestroy(() => {
		scene?.destroy()
	})
</script>

<svelte:window on:resize={setScale} />

<svelte:head>
	<title>Level {id} | Gravity Golf</title>
</svelte:head>

<main>
	<header>
		<BackLink href="/levels">
			Level {id} | Gravity Golf
		</BackLink>
		<div class="forces">
			{#if scale}
				<span
					style="--radius: {FORCE_RADIUS}; --scale: {scale};"
					data-force="gravity"
					data-remaining={forcesRemaining.gravity}
					use:draggable={scene && forcesRemaining.gravity ? addForce(1) : null}
				/>
				<span
					style="--radius: {FORCE_RADIUS}; --scale: {scale};"
					data-force="antigravity"
					data-remaining={forcesRemaining.antigravity}
					use:draggable={scene && forcesRemaining.antigravity
						? addForce(-1)
						: null}
				/>
			{/if}
		</div>
		<div class="stars">
			{#if scale}
				{#each { length: MAX_STARS } as _star, index}
					<img
						class="star"
						src={starImage}
						alt="Star"
						style="--radius: {FORCE_RADIUS}; --scale: {scale};"
						data-hit={index < stars ? '' : undefined}
					/>
				{/each}
			{/if}
		</div>
		<button class="reset" disabled={!scene} on:click={() => scene?.reset()}>
			Press SPACE BAR to restart
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
		top: 1.5rem;
		left: 1.7rem;
		right: 1.7rem;
		z-index: 100;
		pointer-events: none;

		> :global(*) {
			pointer-events: all;
		}
	}

	.forces,
	.stars {
		display: flex;
		align-items: center;
	}

	.forces {
		margin-left: auto;
	}

	[data-force] {
		cursor: move;
		position: relative;
		width: calc((var(--radius) / var(--scale)) * 2px);
		height: calc((var(--radius) / var(--scale)) * 2px);
		background-size: contain;
		border-radius: 50%;
		transition: opacity 0.3s;

		&:not([data-copy])::after {
			content: attr(data-remaining);
			pointer-events: none;
			position: absolute;
			top: 0;
			right: 0;
			padding: 0 calc(1rem / var(--scale));
			font-size: calc(1.6rem / var(--scale));
			color: transparentize(black, 0.5);
			background: transparentize(white, 0.4);
			border-radius: calc(1rem / var(--scale));
			transform: translate(
				calc(2.4rem / var(--scale)),
				calc(-1.6rem / var(--scale))
			);
		}

		& + & {
			margin-left: calc(4rem / var(--scale));
		}
	}

	[data-force='gravity'] {
		background-image: url('../../images/ball.png');
	}

	[data-force='antigravity'] {
		background-image: url('../../images/ball.png');
	}

	[data-remaining='0'] {
		pointer-events: none;
		opacity: 0.5;
	}

	.stars {
		margin: 0 auto 0 3rem;
	}

	.star {
		pointer-events: none;
		width: calc((var(--radius) / var(--scale)) * 2px);
		height: calc((var(--radius) / var(--scale)) * 2px);
		opacity: 0.5;
		transition: opacity 0.3s;

		& + & {
			margin-left: calc(1rem / var(--scale));
		}
	}

	[data-hit] {
		opacity: 1;
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

		> :global(svg) {
			margin-left: 0.7rem;
		}
	}

	.clear {
		margin-left: 1rem;
	}
</style>
