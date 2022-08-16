<script lang="ts">
	import type { LoadOutput } from '@sveltejs/kit'

	import { browser } from '$app/env'
	import { goto } from '$app/navigation'

	import type MaybePromise from '$lib/promise/maybe'

	type Props = $$Generic
	type Load = () => MaybePromise<LoadOutput<Props>>

	export let props: Props | null = null
	export let defaultProps: Props | null = null

	export let load: Load

	$: _props = props as Props
	$: _defaultProps = defaultProps as Props

	$: if (browser && !props) setProps(load)

	const setProps = async (load: Load) => {
		const output = await load()

		if (output.error)
			return alert(
				typeof output.error === 'string' ? output.error : output.error.message
			)

		if (output.redirect) return goto(output.redirect)

		props = (output.props ?? {}) as Props
	}
</script>

{#if _props}
	<slot props={_props} />
{:else if _defaultProps}
	<slot props={_defaultProps} />
{:else}
	<slot name="loading" />
{/if}
