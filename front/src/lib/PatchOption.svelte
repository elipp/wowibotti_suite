<script>
export let config;
const id = `patch-${config.name}`
</script>

<div></div>
<input type="checkbox" id={id} name={id} checked={config.enabled_by_default}>
<label for={id}><span class="monospace">{config.name}</span></label>

<style>
label {
    text-align: left;
    user-select: none;
    width: 10em;
}
input {
    /* width: 2em; */
}
</style>