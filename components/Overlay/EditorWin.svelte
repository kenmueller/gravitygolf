<script lang="ts">
	import { toast } from '@zerodevx/svelte-toast'
	import copy from 'copy-to-clipboard'

	import { page } from '$app/stores'
	import { goto } from '$app/navigation'

	import type RawLevel from '$lib/level/raw'
	import MAX_STARS from '$lib/scene/star/max'
	import hideOverlay from '$lib/overlay/hide'
	import Win from './Win.svelte'
	import replaceWithRounded from '$lib/replaceWithRounded'
	import prefixUrl from '$lib/url/prefix'
	import Save from '../../images/Save.svelte'
	import Reset from '../../images/Reset.svelte'
	import errorFromValue from '$lib/error/from/value'
	import errorFromResponse from '$lib/error/from/response'

	export let stars: number
	export let defaultName: string | null
	export let publishLink: string | null
	export let data: () => RawLevel
	export let reset: () => void
	export let hide: () => void

	$: hitAllStars = stars === MAX_STARS

	let input: HTMLInputElement | null = null
	$: if (publishLink === null && hitAllStars) input?.focus()

	$: name = defaultName ?? ''
	let loading = false

	const restart = () => {
		reset()

		hide()
		hideOverlay()
	}

	const publish = async () => {
		try {
			if (!name || loading) return
			loading = true

			if (publishLink) {
				window.location.href = `${publishLink}${encodeURIComponent(
					JSON.stringify(data(), replaceWithRounded(2))
				)}`
			} else {
				const response = await fetch(prefixUrl('/api/levels/community'), {
					method: 'POST',
					headers: { 'content-type': 'application/json' },
					body: JSON.stringify({ name, data: data() }, replaceWithRounded(2))
				})
				if (!response.ok) throw await errorFromResponse(response)

				const link = `/levels/community/${encodeURIComponent(
					await response.text()
				)}`

				copy(new URL(link, $page.url.origin).href)
				toast.push('Level link copied to clipboard')

				hideOverlay()
				await goto(link)
			}
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
		<input
			placeholder="Name your level"
			disabled={!(publishLink === null && hitAllStars)}
			bind:this={input}
			bind:value={name}
		/>
		<button
			class="publish"
			disabled={!(name && hitAllStars)}
			aria-busy={loading || undefined}
		>
			<Save />
			Publish
		</button>
	</form>
	{#if !hitAllStars}
		<p class="error">You must collect all stars to publish your level</p>
	{/if}
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
		transition: opacity 0.3s;

		&::placeholder {
			color: rgba(white, 0.5);
		}

		&:disabled {
			opacity: 0.7;
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

	.error {
		margin-top: 0.5rem;
		text-align: center;
		color: colors.$red;
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
