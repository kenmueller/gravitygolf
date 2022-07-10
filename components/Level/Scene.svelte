<script lang="ts">
	import { onDestroy } from 'svelte'
	import { fade } from 'svelte/transition'

	import type Level from '$lib/level'
	import type Position from '$lib/position'
	import type Force from '$lib/scene/force'
	import forceRadius from '$lib/scene/force/radius'
	import incrementCommunityLevelProperty from '$lib/level/community/increment'
	import FORCE_DELETE_DIMENSIONS from '$lib/scene/force/delete/dimensions'
	import MAX_STARS from '$lib/scene/star/max'
	import Scene from '$lib/scene'
	import draggable from '$lib/draggable'
	import mobile from '$lib/mobile'
	import view from '$lib/view/store'
	import BackLink from '../../components/Link/Back.svelte'
	import Reset from '../../images/Reset.svelte'
	import Trash from '../../images/Trash.svelte'

	import starImage from '../../images/star.png'

	export let name: string
	export let communityId: string | null = null
	export let level: Level
	export let setStars: (stars: number) => void
	export let back: string
	export let next: string

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas?.getContext('2d')

	$: scene =
		canvas && context && new Scene(canvas, context, level, { setStars, next })

	let hit = false

	$: scene?.addEventListener('hit', newHit => {
		hit = newHit
	})

	$: forcesRemaining = {
		gravity: level.maxGravity - level.defaultGravity.length,
		antigravity: level.maxAntigravity - level.defaultAntigravity.length
	}

	$: scene?.addEventListener('forces', (gravity, antigravity) => {
		forcesRemaining = {
			gravity: level.maxGravity - gravity,
			antigravity: level.maxAntigravity - antigravity
		}
	})

	const addForce = (direction: 1 | -1) => (position: Position) => {
		scene?.addForce(position, direction)
	}

	$: hitStars = MAX_STARS - level.stars.length

	$: scene?.addEventListener('stars', stars => {
		hitStars = stars
	})

	let currentForce: Force | null = null

	$: scene?.addEventListener('force', force => {
		currentForce = force
	})

	$: if (communityId)
		scene?.addEventListener('attempt', () => {
			if (!communityId) return
			incrementCommunityLevelProperty(communityId, 'attempts')
		})

	$: if (communityId)
		scene?.addEventListener('win', () => {
			if (!communityId) return
			incrementCommunityLevelProperty(communityId, 'wins')
		})

	$: radius = forceRadius($mobile)

	onDestroy(() => {
		scene?.destroy()
	})
</script>

<main>
	<header>
		<BackLink href={back}>
			{name}
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
		<div class="forces">
			{#if $view}
				<span
					style="--scale: {radius / $view.scale};"
					data-force="gravity"
					data-remaining={forcesRemaining.gravity}
					data-hit={hit ? '' : undefined}
					use:draggable={scene && !hit && forcesRemaining.gravity
						? addForce(1)
						: null}
				/>
				<span
					style="--scale: {radius / $view.scale};"
					data-force="antigravity"
					data-remaining={forcesRemaining.antigravity}
					data-hit={hit ? '' : undefined}
					use:draggable={scene && !hit && forcesRemaining.antigravity
						? addForce(-1)
						: null}
				/>
			{/if}
		</div>
		<div class="stars">
			{#if $view}
				{#each { length: MAX_STARS } as _star, index (index)}
					<img
						class="star"
						src={starImage}
						alt="Star"
						style="--scale: {radius / $view.scale};"
						data-hit={index < hitStars ? '' : undefined}
					/>
				{/each}
			{/if}
		</div>
		<button
			class="reset"
			disabled={!scene}
			aria-label="Restart"
			on:click={() => scene?.reset()}
		>
			{#if !$mobile}Press SPACE BAR to restart{/if}
			<Reset />
		</button>
		<button
			class="clear"
			disabled={!scene}
			aria-label="Clear"
			on:click={() => scene?.clear()}
		>
			<Trash />
		</button>
	</header>
	<canvas bind:this={canvas} />
	{#if currentForce}
		<p
			class="delete-force"
			style="
				width: {FORCE_DELETE_DIMENSIONS.width}px;
				height: {FORCE_DELETE_DIMENSIONS.height}px;
			"
			transition:fade={{ duration: 300 }}
		>
			<Trash />
			Drag to delete {currentForce.direction === 1 ? '' : 'anti'}gravity
		</p>
	{/if}
</main>

<style lang="scss">
	main {
		position: relative;
		overflow: hidden;
		height: 100%;
	}

	header {
		display: flex;
		align-items: center;
		position: absolute;
		top: 1.5rem;
		left: 2rem;
		right: 2rem;
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
		width: calc(2px * var(--scale));
		height: calc(2px * var(--scale));
		background-size: contain;
		background-repeat: no-repeat;
		border-radius: 50%;
		transition: opacity 0.3s;

		&:not([data-copy])::after {
			content: attr(data-remaining);
			pointer-events: none;
			position: absolute;
			top: 0;
			right: 0;
			padding: 0 calc(0.03rem * var(--scale));
			font-size: calc(0.05rem * var(--scale));
			color: transparentize(black, 0.5);
			background: transparentize(white, 0.4);
			border-radius: calc(0.03rem * var(--scale));
			transform: translate(
				calc(0.07rem * var(--scale)),
				calc(-0.06rem * var(--scale))
			);
		}

		&[data-hit] {
			pointer-events: none;
		}

		& + & {
			margin-left: calc(0.1rem * var(--scale));
		}
	}

	[data-force='gravity'] {
		background-image: url('../../images/gravity.png');
	}

	[data-force='antigravity'] {
		background-image: url('../../images/antigravity.png');
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
		width: calc(2px * var(--scale));
		height: calc(2px * var(--scale));
		opacity: 0.5;
		transition: opacity 0.3s;

		&[data-hit] {
			opacity: 1;
		}

		& + & {
			margin-left: calc(0.03rem * var(--scale));
		}
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

	.delete-force {
		pointer-events: none;
		display: flex;
		justify-content: center;
		align-items: center;
		position: absolute;
		right: 0;
		bottom: 0;
		color: white;
		background: transparentize(white, 0.9);
		border: 0.25rem dashed white;
		z-index: 0;

		> :global(svg) {
			height: 2rem;
			margin-right: 0.5rem;
		}
	}

	canvas {
		position: relative;
		z-index: 50;
	}
</style>
