<script>
export let name;
const id = `patch-${name}`
const is_main_entrypoint = name === 'ClosePetStables__lop_exec';
</script>

<div></div>
<input type="checkbox" id={id} name={id} checked={is_main_entrypoint}>
<label for={id}><span class="monospace">{name}</span></label>

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