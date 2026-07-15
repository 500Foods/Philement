import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="file-manager-container">
        <div class="placeholder-header">
          <fa fa-folder-open></fa>
          <h2>File Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle file storage and organization.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>File Manager module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};