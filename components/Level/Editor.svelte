<script lang="ts">
	import { onDestroy } from 'svelte'
	import { fade } from 'svelte/transition'

	import type Position from '$lib/position'
	import type Force from '$lib/scene/force'
	import type Star from '$lib/scene/star'
	import type ResizableWall from '$lib/scene/wall/resizable'
	import forceRadius from '$lib/scene/force/radius'
	import FORCE_DELETE_DIMENSIONS from '$lib/scene/force/delete/dimensions'
	import MAX_STARS from '$lib/scene/star/max'
	import Editor from '$lib/scene/editor'
	import draggable from '$lib/draggable'
	import mobile from '$lib/mobile'
	import view from '$lib/view/store'
	import BackLink from '../../components/Link/Back.svelte'
	import Reset from '../../images/Reset.svelte'
	import Trash from '../../images/Trash.svelte'

	import starImage from '../../images/star.png'

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas?.getContext('2d')

	$: scene = canvas && context && new Editor(canvas, context)

	let hit = false

	$: scene?.addEventListener('hit', newHit => {
		hit = newHit
	})

	$: playing = false

	let totalGravity = 1
	let totalAntigravity = 1

	let defaultForces = {
		gravity: 0,
		antigravity: 0
	}

	$: forcesRemaining = {
		gravity: totalGravity - defaultForces.gravity,
		antigravity: totalAntigravity - defaultForces.antigravity
	}

	$: scene?.addEventListener('forces', (gravity, antigravity) => {
		defaultForces = { gravity, antigravity }
	})

	const addForce = (direction: 1 | -1) => (position: Position) => {
		scene?.addForce(position, direction)
	}

	let defaultStars = 0

	let stars = MAX_STARS

	$: scene?.addEventListener('stars', newStars => {
		stars = newStars
	})

	$: scene?.addEventListener('defaultStars', newDefaultStars => {
		defaultStars = newDefaultStars
		stars = MAX_STARS - defaultStars
	})

	let currentObject:
		| ({ type: 'force' } & Force)
		| ({ type: 'star' } & Star)
		| ({ type: 'wall' } & ResizableWall)
		| null = null

	$: scene?.addEventListener('force', force => {
		currentObject = force && { ...force, type: 'force' }
	})

	$: scene?.addEventListener('star', star => {
		currentObject = star && { ...star, type: 'star' }
	})

	$: scene?.addEventListener('wall', wall => {
		currentObject = wall && { ...wall, type: 'wall' }
	})

	$: radius = forceRadius($mobile)

	$: scene?.addEventListener('clear', () => {
		totalGravity = totalAntigravity = 1
		defaultForces = { gravity: 0, antigravity: 0 }
		defaultStars = 0
		stars = MAX_STARS
	})

	let maxGravity: HTMLInputElement | null = null
	let maxAntigravity: HTMLInputElement | null = null

	const down = ({ target }: MouseEvent) => {
		if (maxGravity !== target) maxGravity?.blur()
		if (maxAntigravity !== target) maxAntigravity?.blur()
	}

	onDestroy(() => {
		scene?.destroy()
	})
</script>

<svelte:window on:mousedown={down} />

<main>
	<header>
		<BackLink href="/">
			Level Editor
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
		<div class="objects">
			{#if $view}
				<span
					style="--scale: {radius / $view.scale};"
					data-force="gravity"
					data-remaining={forcesRemaining.gravity}
					use:draggable={scene && forcesRemaining.gravity ? addForce(1) : null}
				/>
				<input
					bind:this={maxGravity}
					type="number"
					min={defaultForces.gravity}
					bind:value={totalGravity}
				/>
				<span
					style="--scale: {radius / $view.scale};"
					data-force="antigravity"
					data-remaining={forcesRemaining.antigravity}
					use:draggable={scene && forcesRemaining.antigravity
						? addForce(-1)
						: null}
				/>
				<input
					bind:this={maxAntigravity}
					type="number"
					min={defaultForces.antigravity}
					bind:value={totalAntigravity}
				/>
				<span class="wall" use:draggable={scene ? scene.addObstacle : null} />
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
						aria-disabled={index >= (hit ? stars : MAX_STARS - defaultStars) ||
							undefined}
						data-hit={index < stars ? '' : undefined}
						use:draggable={scene && !hit && index < MAX_STARS - defaultStars
							? scene.addStar
							: null}
					/>
				{/each}
			{/if}
		</div>
		<button
			class="play"
			disabled={!scene || playing}
			aria-label="Play"
			on:click={() => {
				playing = true
				scene?.play()
			}}
		>
			{playing ? '' : 'Play'}
		</button>
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
	{#if currentObject}
		<p
			class="delete-force"
			style="
				width: {FORCE_DELETE_DIMENSIONS.width}px;
				height: {FORCE_DELETE_DIMENSIONS.height}px;
			"
			transition:fade={{ duration: 300 }}
		>
			<Trash />
			Drag to delete {currentObject.type === 'force'
				? currentObject.direction === 1
					? 'gravity'
					: 'antigravity'
				: currentObject.type}
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

	.objects,
	.stars {
		display: flex;
		align-items: center;
	}

	.objects {
		margin-left: auto;
	}

	[data-force],
	.wall,
	.star {
		z-index: 200;
	}

	[data-force] {
		flex-shrink: 0;
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

	input {
		overflow: hidden;
		max-width: 5rem;
		width: 100%;
		margin: 0 2rem 0 0.8rem;
		padding: 0.3rem 0.5rem;
		color: white;
		background: rgba(white, 0.1);
		border-radius: 0.5rem;

		&::placeholder {
			color: rgba(white, 0.5);
		}
	}

	.wall {
		flex-shrink: 0;
		cursor: move;
		width: 20px;
		height: 35px;
		background: white;
	}

	.stars {
		margin: 0 auto 0 3rem;
	}

	.star {
		width: calc(2px * var(--scale));
		height: calc(2px * var(--scale));
		opacity: 0.5;
		transition: opacity 0.3s;

		&[aria-disabled] {
			opacity: 0.5;
		}

		&:not([aria-disabled]):hover {
			cursor: move;
		}

		& + & {
			margin-left: calc(0.03rem * var(--scale));
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
