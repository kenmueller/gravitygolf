<script lang="ts">
	import { goto } from '$app/navigation'

	import type RawLevel from '$lib/level/raw'
	import hideOverlay from '$lib/overlay/hide'
	import Win from './Win.svelte'
	import replaceWithRounded from '$lib/replaceWithRounded'
	import Save from '../../images/Save.svelte'
	import Reset from '../../images/Reset.svelte'
	import errorFromValue from '$lib/error/from/value'
	import HttpError from '$lib/error'

	export let stars: number
	export let data: () => RawLevel
	export let reset: () => void

	let input: HTMLInputElement | null = null
	$: input?.focus()

	let name = ''
	let loading = false

	const restart = () => {
		reset()
		hideOverlay()
	}

	const publish = async () => {
		try {
			if (!name) return

			loading = true

			const response = await fetch('/communityLevels', {
				method: 'POST',
				headers: { 'content-type': 'application/json' },
				body: JSON.stringify({ name, data: data() }, replaceWithRounded(2))
			})

			if (!response.ok)
				throw new HttpError(response.status, await response.text())

			hideOverlay()

			await goto('/levels/community')
		} catch (value) {
			console.error(value)
			alert(errorFromValue(value).message)
		} finally {
			loading = false
		}
	}
</script>

<Win {stars}>
	<form on:submit|preventDefault={publish}>
		<input placeholder="Name your level" bind:this={input} bind:value={name} />
		<button class="publish" disabled={!name} aria-busy={loading || undefined}>
			<Save />
			Publish
		</button>
	</form>
	<button class="restart" on:click={restart}>
		<Reset />
		Restart
	</button>
</Win>

<style lang="scss">
	form {
		display: flex;
		align-items: stretch;
		margin-top: 2.5rem;
	}

	input {
		max-width: 20rem;
		width: 100%;
		padding: 0 0.8rem;
		font-size: 1rem;
		color: white;
		background: rgba(white, 0.1);
		border-radius: 0.5rem;

		&::placeholder {
			color: rgba(white, 0.5);
		}
	}

	button {
		display: flex;
		align-items: center;
		padding: 0.5rem 1rem;
		font-size: 1rem;
		color: white;
		background: colors.$blue;
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		> :global(svg) {
			margin-right: 0.5rem;
			transform: translateY(-1px);
		}

		&:disabled,
		&[aria-busy] {
			opacity: 0.5;
		}

		&:not(:disabled):not([aria-busy]):hover {
			opacity: 0.7;
		}
	}

	.publish {
		margin-left: 1rem;

		> :global(svg) {
			height: 1.7rem;
		}
	}

	.restart {
		margin-top: 2rem;

		> :global(svg) {
			height: 1.4rem;
		}
	}
</style>
