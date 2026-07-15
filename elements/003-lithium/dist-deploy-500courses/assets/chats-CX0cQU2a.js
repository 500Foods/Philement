import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="chats-container">
        <div class="placeholder-header">
          <fa fa-comments></fa>
          <h2>Chats</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle chat conversations and history.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Chats module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};