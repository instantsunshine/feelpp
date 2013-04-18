var NAVTREE =
[
  [ "Feel++", "index.html", [
    [ "Introduction", "index.html", null ],
    [ "Getting Started", "usergroup0.html", "usergroup0" ],
    [ "Quick Reference", "usergroup1.html", "usergroup1" ],
    [ "Class List", "annotated.html", "annotated" ]
  ] ]
];

var NAVTREEINDEX =
[
".html",
"classFeel_1_1Advection.html#aca1fd840d77e40563ba72afb462fa470",
"classFeel_1_1Backend.html#afaf1a0291a7da8c98f30ae4cac8bea4c",
"classFeel_1_1CRBDB.html#a6b26667b5d61e737479bcc9d0782b43e",
"classFeel_1_1CRBModelTrilinear.html#adb0c6db8ef01f03f914d790d94ba87f0",
"classFeel_1_1DataMap.html#acefa415e83277fcfd63f7e43625b47dc",
"classFeel_1_1ExporterGmsh.html#a47b8c426c3c40fd8fa152a6795ce896e",
"classFeel_1_1GeoElement1D.html#ab7f5d884a964c8fc80d6fdea48a8b2da",
"classFeel_1_1GeoMap.html#a9e9913539c29dac36134dc75e363d295",
"classFeel_1_1GeoND.html#af7fb873f66580e200eb9ef74b65f0a49",
"classFeel_1_1Info.html#ae0c3a22c420cbbc1da737becc3588277",
"classFeel_1_1MatrixSparse.html#a83fd676ed777d81fc3191cfa880c62bd",
"classFeel_1_1Mesh3D.html#aafa67d57b2a85ae4036db14c5c875b42",
"classFeel_1_1ParameterSpace_1_1Element.html#ac98d07dd8f7b70e16ccb9a01abf56b9c",
"classFeel_1_1PolynomialSet.html#a61bfef705fa9ed3f575e417aff3799ad",
"classFeel_1_1SolverBase.html",
"classFeel_1_1Sound.html#adecea58e745ad03afb106df392c82673",
"classFeel_1_1VectorBlock.html#a2acc5d7403533a700eb70aaf147ee701",
"classFeel_1_1dyna_1_1Jacobi.html#af364dd7e37383cbbff85b7dc9765fca6",
"classLShape.html#a3322994950a72e0643be5ca69dcb049e",
"structFeel_1_1BoundingBox.html#ace8132883dc982fb392dc561056858d1"
];

var navTreeSubIndices = new Array();

function getData(varName)
{
  var i = varName.lastIndexOf('/');
  var n = i>=0 ? varName.substring(i+1) : varName;
  return eval(n.replace(/\-/g,'_'));
}

function stripPath(uri)
{
  return uri.substring(uri.lastIndexOf('/')+1);
}

function stripPath2(uri)
{
  var i = uri.lastIndexOf('/');
  var s = uri.substring(i+1);
  var m = uri.substring(0,i+1).match(/\/d\w\/d\w\w\/$/);
  return m ? uri.substring(i-6) : s;
}

function localStorageSupported()
{
  return 'localStorage' in window && window['localStorage'] !== null;
}

function storeLink(link)
{
  if (!$("#nav-sync").hasClass('sync') && localStorageSupported()) {
      window.localStorage.setItem('navpath',link);
  }
}

function deleteLink()
{
  if (localStorageSupported()) {
    window.localStorage.setItem('navpath','');
  } 
}

function cachedLink()
{
  if (localStorageSupported()) {
    return window.localStorage.getItem('navpath');
  } else {
    return '';
  }
}

function getScript(scriptName,func,show)
{
  var head = document.getElementsByTagName("head")[0]; 
  var script = document.createElement('script');
  script.id = scriptName;
  script.type = 'text/javascript';
  script.onload = func; 
  script.src = scriptName+'.js'; 
  if ($.browser.msie && $.browser.version<=8) { 
    // script.onload does work with older versions of IE
    script.onreadystatechange = function() {
      if (script.readyState=='complete' || script.readyState=='loaded') { 
        func(); if (show) showRoot(); 
      }
    }
  }
  head.appendChild(script); 
}

function createIndent(o,domNode,node,level)
{
  if (node.parentNode && node.parentNode.parentNode) {
    createIndent(o,domNode,node.parentNode,level+1);
  }
  var imgNode = document.createElement("img");
  imgNode.width = 16;
  imgNode.height = 22;
  if (level==0 && node.childrenData) {
    node.plus_img = imgNode;
    node.expandToggle = document.createElement("a");
    node.expandToggle.href = "javascript:void(0)";
    node.expandToggle.onclick = function() {
      if (node.expanded) {
        $(node.getChildrenUL()).slideUp("fast");
        if (node.isLast) {
          node.plus_img.src = node.relpath+"ftv2plastnode.png";
        } else {
          node.plus_img.src = node.relpath+"ftv2pnode.png";
        }
        node.expanded = false;
      } else {
        expandNode(o, node, false, false);
      }
    }
    node.expandToggle.appendChild(imgNode);
    domNode.appendChild(node.expandToggle);
  } else {
    domNode.appendChild(imgNode);
  }
  if (level==0) {
    if (node.isLast) {
      if (node.childrenData) {
        imgNode.src = node.relpath+"ftv2plastnode.png";
      } else {
        imgNode.src = node.relpath+"ftv2lastnode.png";
        domNode.appendChild(imgNode);
      }
    } else {
      if (node.childrenData) {
        imgNode.src = node.relpath+"ftv2pnode.png";
      } else {
        imgNode.src = node.relpath+"ftv2node.png";
        domNode.appendChild(imgNode);
      }
    }
  } else {
    if (node.isLast) {
      imgNode.src = node.relpath+"ftv2blank.png";
    } else {
      imgNode.src = node.relpath+"ftv2vertline.png";
    }
  }
  imgNode.border = "0";
}

function newNode(o, po, text, link, childrenData, lastNode)
{
  var node = new Object();
  node.children = Array();
  node.childrenData = childrenData;
  node.depth = po.depth + 1;
  node.relpath = po.relpath;
  node.isLast = lastNode;

  node.li = document.createElement("li");
  po.getChildrenUL().appendChild(node.li);
  node.parentNode = po;

  node.itemDiv = document.createElement("div");
  node.itemDiv.className = "item";

  node.labelSpan = document.createElement("span");
  node.labelSpan.className = "label";

  createIndent(o,node.itemDiv,node,0);
  node.itemDiv.appendChild(node.labelSpan);
  node.li.appendChild(node.itemDiv);

  var a = document.createElement("a");
  node.labelSpan.appendChild(a);
  node.label = document.createTextNode(text);
  node.expanded = false;
  a.appendChild(node.label);
  if (link) {
    var url;
    if (link.substring(0,1)=='^') {
      url = link.substring(1);
      link = url;
    } else {
      url = node.relpath+link;
    }
    a.className = stripPath(link.replace('#',':'));
    if (link.indexOf('#')!=-1) {
      var aname = '#'+link.split('#')[1];
      var srcPage = stripPath($(location).attr('pathname'));
      var targetPage = stripPath(link.split('#')[0]);
      a.href = srcPage!=targetPage ? url : '#';
      a.onclick = function(){
        storeLink(link);
        if (!$(a).parent().parent().hasClass('selected'))
        {
          $('.item').removeClass('selected');
          $('.item').removeAttr('id');
          $(a).parent().parent().addClass('selected');
          $(a).parent().parent().attr('id','selected');
        }
        var pos, anchor = $(aname), docContent = $('#doc-content');
        if (anchor.parent().attr('class')=='memItemLeft') {
          pos = anchor.parent().position().top;
        } else if (anchor.position()) {
          pos = anchor.position().top;
        }
        if (pos) {
          var dist = Math.abs(Math.min(
                     pos-docContent.offset().top,
                     docContent[0].scrollHeight-
                     docContent.height()-docContent.scrollTop()));
          docContent.animate({
            scrollTop: pos + docContent.scrollTop() - docContent.offset().top
          },Math.max(50,Math.min(500,dist)),function(){
            window.location.replace(aname);
          });
        }
      };
    } else {
      a.href = url;
      a.onclick = function() { storeLink(link); }
    }
  } else {
    if (childrenData != null) 
    {
      a.className = "nolink";
      a.href = "javascript:void(0)";
      a.onclick = node.expandToggle.onclick;
    }
  }

  node.childrenUL = null;
  node.getChildrenUL = function() {
    if (!node.childrenUL) {
      node.childrenUL = document.createElement("ul");
      node.childrenUL.className = "children_ul";
      node.childrenUL.style.display = "none";
      node.li.appendChild(node.childrenUL);
    }
    return node.childrenUL;
  };

  return node;
}

function showRoot()
{
  var headerHeight = $("#top").height();
  var footerHeight = $("#nav-path").height();
  var windowHeight = $(window).height() - headerHeight - footerHeight;
  (function (){ // retry until we can scroll to the selected item
    try {
      var navtree=$('#nav-tree');
      navtree.scrollTo('#selected',0,{offset:-windowHeight/2});
    } catch (err) {
      setTimeout(arguments.callee, 0);
    }
  })();
}

function expandNode(o, node, imm, showRoot)
{
  if (node.childrenData && !node.expanded) {
    if (typeof(node.childrenData)==='string') {
      var varName    = node.childrenData;
      getScript(node.relpath+varName,function(){
        node.childrenData = getData(varName);
        expandNode(o, node, imm, showRoot);
      }, showRoot);
    } else {
      if (!node.childrenVisited) {
        getNode(o, node);
      } if (imm || ($.browser.msie && $.browser.version>8)) { 
        // somehow slideDown jumps to the start of tree for IE9 :-(
        $(node.getChildrenUL()).show();
      } else {
        $(node.getChildrenUL()).slideDown("fast");
      }
      if (node.isLast) {
        node.plus_img.src = node.relpath+"ftv2mlastnode.png";
      } else {
        node.plus_img.src = node.relpath+"ftv2mnode.png";
      }
      node.expanded = true;
    }
  }
}

function glowEffect(n,duration)
{
  n.addClass('glow').delay(duration).queue(function(next){
    $(this).removeClass('glow');next();
  });
}

function highlightAnchor()
{
  var anchor = $($(location).attr('hash'));
  if (anchor.parent().attr('class')=='memItemLeft'){
    var rows = $('.memberdecls tr[class$="'+
               window.location.hash.substring(1)+'"]');
    glowEffect(rows.children(),300); // member without details
  } else if (anchor.parents().slice(2).prop('tagName')=='TR') {
    glowEffect(anchor.parents('div.memitem'),1000); // enum value
  } else if (anchor.parent().attr('class')=='fieldtype'){
    glowEffect(anchor.parent().parent(),1000); // struct field
  } else if (anchor.parent().is(":header")) {
    glowEffect(anchor.parent(),1000); // section header
  } else {
    glowEffect(anchor.next(),1000); // normal member
  }
}

function selectAndHighlight(hash,n)
{
  var a;
  if (hash) {
    var link=stripPath($(location).attr('pathname'))+':'+hash.substring(1);
    a=$('.item a[class$="'+link+'"]');
  }
  if (a && a.length) {
    a.parent().parent().addClass('selected');
    a.parent().parent().attr('id','selected');
    highlightAnchor();
  } else if (n) {
    $(n.itemDiv).addClass('selected');
    $(n.itemDiv).attr('id','selected');
  }
  showRoot();
}

function showNode(o, node, index, hash)
{
  if (node && node.childrenData) {
    if (typeof(node.childrenData)==='string') {
      var varName    = node.childrenData;
      getScript(node.relpath+varName,function(){
        node.childrenData = getData(varName);
        showNode(o,node,index,hash);
      },true);
    } else {
      if (!node.childrenVisited) {
        getNode(o, node);
      }
      $(node.getChildrenUL()).show();
      if (node.isLast) {
        node.plus_img.src = node.relpath+"ftv2mlastnode.png";
      } else {
        node.plus_img.src = node.relpath+"ftv2mnode.png";
      }
      node.expanded = true;
      var n = node.children[o.breadcrumbs[index]];
      if (index+1<o.breadcrumbs.length) {
        showNode(o,n,index+1,hash);
      } else {
        if (typeof(n.childrenData)==='string') {
          var varName = n.childrenData;
          getScript(n.relpath+varName,function(){
            n.childrenData = getData(varName);
            node.expanded=false;
            showNode(o,node,index,hash); // retry with child node expanded
          },true);
        } else {
          var rootBase = stripPath(o.toroot.replace(/\..+$/, ''));
          if (rootBase=="index" || rootBase=="pages") {
            expandNode(o, n, true, true);
          }
          selectAndHighlight(hash,n);
        }
      }
    }
  } else {
    selectAndHighlight(hash);
  }
}

function getNode(o, po)
{
  po.childrenVisited = true;
  var l = po.childrenData.length-1;
  for (var i in po.childrenData) {
    var nodeData = po.childrenData[i];
    po.children[i] = newNode(o, po, nodeData[0], nodeData[1], nodeData[2],
      i==l);
  }
}

function gotoNode(o,subIndex,root,hash,relpath)
{
  var nti = navTreeSubIndices[subIndex][root+hash];
  o.breadcrumbs = $.extend(true, [], nti ? nti : navTreeSubIndices[subIndex][root]);
  if (!o.breadcrumbs && root!=NAVTREE[0][1]) { // fallback: show index
    navTo(o,NAVTREE[0][1],"",relpath);
    $('.item').removeClass('selected');
    $('.item').removeAttr('id');
  }
  if (o.breadcrumbs) {
    o.breadcrumbs.unshift(0); // add 0 for root node
    showNode(o, o.node, 0, hash);
  }
}

function navTo(o,root,hash,relpath)
{
  var link = cachedLink();
  if (link) {
    var parts = link.split('#');
    root = parts[0];
    if (parts.length>1) hash = '#'+parts[1];
    else hash='';
  }
  if (root==NAVTREE[0][1]) {
    $('#nav-sync').css('top','30px');
  } else {
    $('#nav-sync').css('top','5px');
  }
  if (hash.match(/^#l\d+$/)) {
    var anchor=$('a[name='+hash.substring(1)+']');
    glowEffect(anchor.parent(),1000); // line number
    hash=''; // strip line number anchors
    //root=root.replace(/_source\./,'.'); // source link to doc link
  }
  var url=root+hash;
  var i=-1;
  while (NAVTREEINDEX[i+1]<=url) i++;
  if (navTreeSubIndices[i]) {
    gotoNode(o,i,root,hash,relpath)
  } else {
    getScript(relpath+'navtreeindex'+i,function(){
      navTreeSubIndices[i] = eval('NAVTREEINDEX'+i);
      if (navTreeSubIndices[i]) {
        gotoNode(o,i,root,hash,relpath);
      }
    },true);
  }
}

function toggleSyncButton(relpath)
{
  var navSync = $('#nav-sync');
  if (navSync.hasClass('sync')) {
    navSync.removeClass('sync');
    navSync.html('<img src="'+relpath+'sync_off.png"/>');
    storeLink(stripPath2($(location).attr('pathname'))+$(location).attr('hash'));
  } else {
    navSync.addClass('sync');
    navSync.html('<img src="'+relpath+'sync_on.png"/>');
    deleteLink();
  }
}

function initNavTree(toroot,relpath)
{
  var o = new Object();
  o.toroot = toroot;
  o.node = new Object();
  o.node.li = document.getElementById("nav-tree-contents");
  o.node.childrenData = NAVTREE;
  o.node.children = new Array();
  o.node.childrenUL = document.createElement("ul");
  o.node.getChildrenUL = function() { return o.node.childrenUL; };
  o.node.li.appendChild(o.node.childrenUL);
  o.node.depth = 0;
  o.node.relpath = relpath;
  o.node.expanded = false;
  o.node.isLast = true;
  o.node.plus_img = document.createElement("img");
  o.node.plus_img.src = relpath+"ftv2pnode.png";
  o.node.plus_img.width = 16;
  o.node.plus_img.height = 22;

  if (localStorageSupported()) {
    var navSync = $('#nav-sync');
    if (cachedLink()) {
      navSync.html('<img src="'+relpath+'sync_off.png"/>');
      navSync.removeClass('sync');
    } else {
      navSync.html('<img src="'+relpath+'sync_on.png"/>');
    }
    navSync.click(function(){ toggleSyncButton(relpath); });
  }

  navTo(o,toroot,window.location.hash,relpath);

  $(window).bind('hashchange', function(){
     if (window.location.hash && window.location.hash.length>1){
       var a;
       if ($(location).attr('hash')){
         var clslink=stripPath($(location).attr('pathname'))+':'+
                               $(location).attr('hash').substring(1);
         a=$('.item a[class$="'+clslink+'"]');
       }
       if (a==null || !$(a).parent().parent().hasClass('selected')){
         $('.item').removeClass('selected');
         $('.item').removeAttr('id');
       }
       var link=stripPath2($(location).attr('pathname'));
       navTo(o,link,$(location).attr('hash'),relpath);
     }
  })

  $(window).load(showRoot);
}

