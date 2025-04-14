
export class Socket {

  constructor(kind: 'req' | 'rep' | 'pub' | 'sub');

  /**
   * Dial remote NNG server
   *
   * Example: tcp://127.0.0.1:5555
   */
  Dial(url: string);

  /**
   * Send
   */
  Send(msg: string);

  /**
   * Recv
   *
   */
  async Recv(): Promise<string>;

  /**
   * Close session
   *
   */
  Close();

}
