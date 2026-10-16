/* Reuse the real webOS dispatcher and dependency stubs, without executing
 * unrelated ACB/audio-selection scenarios from the existing fixture. */
#define main video_acb_fixture_main
#include "video_acb_lg.c"
#undef main
int main(void) {
  /* An already queued native error belongs to the pipeline being closed,
   * not to the DTS/DV pipeline which will be created immediately after it. */
  sessao=41;closing_session=sessao;reconErroPend=reconErroRede=1;
  dtsSessao=NULL;ligado=0;posSeg=5;
  pararSessao();
  assert(buffer_stops==1 && sessao==42);
  assert(!reconErroPend && !reconErroRede && !falhou);
  /* Genuine errors from the replacement session must still be handled. */
  eventoPayload("{\"error\":{\"errorCode\":300,\"errorText\":\"Network Error\"}}",sessao);
  assert(reconErroPend && reconErroRede);
  reconErroPend=reconErroRede=0;posSeg=0;
  puts("Native buffer teardown: invalidate callbacks first, discard pending old error, preserve real new errors passed");
  return 0;
}
