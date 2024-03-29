<script lang="ts">
	import { onDestroy } from 'svelte'
	import { fade } from 'svelte/transition'

	import { page } from '$app/stores'

	import type Position from '$lib/position'
	import type Force from '$lib/scene/force'
	import type Star from '$lib/scene/star'
	import type ResizableWall from '$lib/scene/wall/resizable'
	import type RawLevel from '$lib/level/raw'
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
	import Play from '../../images/Play.svelte'
	import Edit from '../../images/Edit.svelte'

	import starImage from '../../images/star.png'

	$: query = $page.url.searchParams

	$: defaultName = query.get('name') || null
	$: backLink = query.get('back') || null
	$: publishLink = query.get('publish') || null
	$: initialData = JSON.parse(query.get('data') || 'null') as RawLevel | null

	let canvas: HTMLCanvasElement | null = null
	$: context = canvas?.getContext('2d')

	$: scene =
		canvas &&
		context &&
		new Editor(canvas, context, { defaultName, publishLink, initialData })

	let hit = false

	$: scene?.addEventListener('hit', newHit => {
		hit = newHit
	})

	$: playing = false

	$: totalGravity =
		(scene?.initialData.maxGravity ?? 0) +
		(scene?.initialData.fixedGravity.length ?? 0)

	$: totalAntigravity =
		(scene?.initialData.maxAntigravity ?? 0) +
		(scene?.initialData.fixedAntigravity.length ?? 0)

	$: fixedForces = {
		gravity: scene?.initialData.fixedGravity.length ?? 0,
		antigravity: scene?.initialData.fixedAntigravity.length ?? 0
	}

	$: forcesRemaining = {
		gravity: totalGravity - fixedForces.gravity,
		antigravity: totalAntigravity - fixedForces.antigravity
	}

	$: scene?.addEventListener('forces', (gravity, antigravity) => {
		fixedForces = { gravity, antigravity }
	})

	const addForce =
		(direction: 1 | -1, fixed: boolean) => (position: Position) => {
			scene?.addForce(position, direction, fixed)
		}

	$: fixedStars = scene?.initialData.stars.length ?? 0
	$: stars = MAX_STARS - fixedStars

	$: scene?.addEventListener('fixedStars', newFixedStars => {
		fixedStars = newFixedStars
	})

	$: scene?.addEventListener('stars', newStars => {
		stars = newStars
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
		if (!scene) return

		totalGravity =
			scene.initialData.maxGravity + scene.initialData.fixedGravity.length

		scene.updateMaxGravities(totalGravity)

		totalAntigravity =
			scene.initialData.maxAntigravity +
			scene.initialData.fixedAntigravity.length

		scene.updateMaxAntigravities(totalAntigravity)

		fixedForces = {
			gravity: scene.initialData.fixedGravity.length,
			antigravity: scene.initialData.fixedAntigravity.length
		}

		fixedStars = scene.initialData.stars.length

		playing = false
	})

	$: scene?.addEventListener('reset', () => {
		playing = false
	})

	let isWinOverlayShowing = false

	$: scene?.addEventListener('win', showing => {
		isWinOverlayShowing = showing
	})

	let maxGravity: HTMLInputElement | null = null
	let maxAntigravity: HTMLInputElement | null = null

	const down = ({ target }: MouseEvent) => {
		for (const input of [maxGravity, maxAntigravity])
			if (input && input !== target) input?.blur()
	}

	onDestroy(() => {
		scene?.destroy()
	})
</script>

<svelte:body on:mousedown={down} />

<main>
	<header>
		<BackLink
			href={backLink || '/'}
			message={isWinOverlayShowing && publishLink
				? null
				: 'All progress will be lost. Are you sure?'}
			focusable={false}
		>
			<span>Level&nbsp;</span>
			Editor
			{#if !$mobile}
				<span>| Gravity Golf</span>
			{/if}
		</BackLink>
		<div class="objects">
			{#if $view}
				<span
					class="force-container"
					aria-label={playing ? 'Gravity force' : 'Unmovable gravity force'}
					data-balloon-pos={playing ? undefined : 'down'}
				>
					<span
						class:playing
						style="--scale: {radius / $view.scale};"
						data-force="gravity"
						data-remaining={forcesRemaining.gravity}
						use:draggable={scene && forcesRemaining.gravity
							? addForce(1, !playing)
							: null}
					/>
				</span>
				<input
					bind:this={maxGravity}
					type="number"
					min={fixedForces.gravity}
					bind:value={totalGravity}
					on:input={() => scene?.updateMaxGravities(totalGravity)}
				/>
				<span
					class="force-container"
					aria-label={playing
						? 'Antigravity force'
						: 'Unmovable antigravity force'}
					data-balloon-pos={playing ? undefined : 'down'}
				>
					<span
						class:playing
						style="--scale: {radius / $view.scale};"
						data-force="antigravity"
						data-remaining={forcesRemaining.antigravity}
						use:draggable={scene && forcesRemaining.antigravity
							? addForce(-1, !playing)
							: null}
					/>
				</span>
				<input
					bind:this={maxAntigravity}
					type="number"
					min={fixedForces.antigravity}
					bind:value={totalAntigravity}
					on:input={() => scene?.updateMaxAntigravities(totalAntigravity)}
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
						aria-disabled={index >= (hit ? stars : MAX_STARS - fixedStars) ||
							undefined}
						data-hit={index < stars ? '' : undefined}
						use:draggable={scene && !hit && index < MAX_STARS - fixedStars
							? scene.addStar
							: null}
					/>
				{/each}
			{/if}
		</div>
		<button
			class="mode"
			disabled={!scene}
			tabindex={-1}
			on:click={() => (playing = !playing)}
		>
			{#if playing}
				<Edit />
				<span>Edit</span>
			{:else}
				<Play />
				<span>Play</span>
			{/if}
		</button>
		<button
			class="reset"
			disabled={!scene}
			tabindex={-1}
			aria-label="Restart"
			on:click={() => scene?.reset()}
		>
			{#if !$mobile}
				<span>Press SPACE BAR to restart</span>
			{/if}
			<Reset />
		</button>
		<button
			class="clear"
			disabled={!scene}
			tabindex={-1}
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
		left: 1rem;
		right: 1rem;
		z-index: 100;
		pointer-events: none;

		> :global(*) {
			pointer-events: all;
		}

		> :global a {
			margin-right: 1rem;

			@media (min-width: 775px) {
				margin-right: 0;
			}

			> span {
				display: none;

				@media (min-width: 775px) {
					display: block;
				}
			}
		}

		@media (min-width: 1070px) {
			left: 2rem;
			right: 2rem;
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

	.force-container {
		flex-shrink: 0;
	}

	[data-force] {
		cursor: move;
		display: block;
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
			color: rgba(black, 0.5);
			background: rgba(white, 0.6);
			border-radius: calc(0.03rem * var(--scale));
			transform: translate(
				calc(0.07rem * var(--scale)),
				calc(-0.06rem * var(--scale))
			);
		}
	}

	.playing[data-force='gravity'] {
		background-image: url('../../images/gravity.png');
	}

	[data-force='gravity'] {
		background-image: url('../../images/fixed-gravity.png');
	}

	.playing[data-force='antigravity'] {
		background-image: url('../../images/antigravity.png');
	}

	[data-force='antigravity'] {
		background-image: url('../../images/fixed-antigravity.png');
	}

	[data-remaining='0'] {
		pointer-events: none;
		opacity: 0.5;
	}

	input {
		overflow: hidden;
		max-width: 5rem;
		width: 100%;
		margin: 0 1.5rem 0 0.8rem;
		padding: 0.3rem 0.5rem;
		color: white;
		background: rgba(white, 0.1);
		border-radius: 0.5rem;

		&::placeholder {
			color: rgba(white, 0.5);
		}

		@media (min-width: 71.25rem) {
			margin: 0 2rem 0 0.8rem;
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
		margin: 0 auto 0 1.5rem;

		@media (min-width: 71.25rem) {
			margin: 0 auto 0 3rem;
		}
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
			flex-shrink: 0;
			width: 1.5rem;
		}
	}

	.mode {
		display: flex;
		justify-content: center;
		align-items: center;
		margin-right: auto;
		padding: 0 1rem;

		@media (min-width: 1200px) {
			min-width: 3.75rem;
			padding: 0;
		}

		> span {
			display: none;
			margin-left: 0.5rem;

			@media (min-width: 1200px) {
				display: block;
			}
		}
	}

	.reset {
		display: flex;
		align-items: center;

		> span {
			margin-right: 0.7rem;
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
		background: rgba(white, 0.1);
		border: 0.25rem dashed white;
		z-index: 0;

		> :global(svg) {
			flex-shrink: 0;
			height: 2rem;
			margin-right: 0.5rem;
		}
	}

	canvas {
		position: relative;
		z-index: 50;
	}
</style>
